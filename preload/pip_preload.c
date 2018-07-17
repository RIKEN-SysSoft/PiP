/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
  * $PIP_license: Redistribution and use in source and binary forms,
  * with or without modification, are permitted provided that the following
  * conditions are met:
  * 
  *     Redistributions of source code must retain the above copyright
  *     notice, this list of conditions and the following disclaimer.
  * 
  *     Redistributions in binary form must reproduce the above copyright
  *     notice, this list of conditions and the following disclaimer in
  *     the documentation and/or other materials provided with the
  *     distribution.
  * 
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  * 
  * The views and conclusions contained in the software and documentation
  * are those of the authors and should not be interpreted as representing
  * official policies, either expressed or implied, of the FreeBSD Project.$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <sched.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>

//#define DEBUG
#include <pip_clone.h>
#include <pip.h>
#include <pip_debug.h>

//#define CHECK_TLS
#ifdef CHECK_TLS
#include <asm/prctl.h>
#include <asm/ldt.h>
#include <sys/prctl.h>
#include <pthread.h>
#endif

pip_clone_t pip_clone_info = { 0 }; /* refered by piplib */

typedef
int(*clone_syscall_t)(int(*)(void*), void*, int, void*, pid_t*, void*, pid_t*);

static clone_syscall_t pip_clone_orig = NULL;

int __clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  pid_t pip_gettid( void ) {
    return (pid_t) syscall( (long int) SYS_gettid );
  }
  clone_syscall_t pip_get_clone( void ) {
    if( pip_clone_orig == NULL ) {
      pip_clone_orig = (clone_syscall_t) dlsym( RTLD_NEXT, "__clone" );
    }
    return pip_clone_orig;
  }

  pid_t		 tid = pip_gettid();
  pip_spinlock_t oldval;
  int 		 retval = -1;

  DBGF( "tid=%d", tid );
  while( 1 ) {
    oldval = pip_spin_trylock_wv( &pip_clone_info.lock, PIP_LOCK_OTHERWISE );
    DBGF( "oldval=%d", oldval );
    switch( oldval ) {
    case PIP_LOCK_UNLOCKED:
      /* lock succeeded */
      DBG;
      goto lock_ok;
    case PIP_LOCK_OTHERWISE:
      /* busy-waiting */
      DBG;
      continue;
    default:
      if( oldval == tid ) {
	DBG;
	goto lock_ok; /* locked by myself */
      }
      DBG;
      break;
    }
  }
 lock_ok:
  {
    va_list ap;
    va_start( ap, args );
    pid_t *ptid = va_arg( ap, pid_t*);
    void  *tls  = va_arg( ap, void*);
    pid_t *ctid = va_arg( ap, pid_t*);

    if( pip_clone_orig == NULL ) {
      if( ( pip_clone_orig = pip_get_clone() ) == NULL ) {
	DBGF( "!!! Original clone() NOT FOUND" );
	errno = ENOSYS;
	goto error;
      }
    }
    if( oldval == PIP_LOCK_UNLOCKED ) {
      DBGF( "!!! Original clone() is used" );
      retval = pip_clone_orig( fn, child_stack, flags, args, ptid, tls, ctid);

#ifdef CHECK_TLS
#ifdef __x86_64__
      if( flags & CLONE_SETTLS ) {
	unsigned long fsaddr;
	arch_prctl( ARCH_GET_FS, &fsaddr );
	pip_print_maps();
	fprintf( stderr,
		 "TLS=%p  tls.base_addr=%u  FS=%p  PThread=%p  STACK=%p\n",
		 (void*) tls, ((struct user_desc *) tls)->base_addr,
		 (void*) fsaddr, (void*) pthread_self(), child_stack );
      } else {
	fprintf( stderr, "CLONE_SETTLS is not set\n" );
      }
#endif
#endif

    } else if( oldval == tid ) {
      DBGF( "!!! clone() wrapper" );
#ifdef DEBUG
      int oldflags = flags;
#endif

      flags &= ~(CLONE_FS);	 /* 0x00200 */
      flags &= ~(CLONE_FILES);	 /* 0x00400 */
      flags &= ~(CLONE_SIGHAND); /* 0x00800 */
      flags &= ~(CLONE_THREAD);	 /* 0x10000 */
      flags &= ~0xff;
      flags |= SIGCHLD;
      flags |= CLONE_VM;
      /* do not reset the CLONE_SETTLS flag */
      flags |= CLONE_SETTLS;
      flags |= CLONE_PTRACE;

      errno = 0;
      DBGF( ">>>> clone(flags: 0x%x -> 0x%x)@%p  STACK=%p, TLS=%p",
	    oldflags, flags, fn, child_stack, tls );
      retval = pip_clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
      DBGF( "<<<< clone()=%d (errno=%d)", retval, errno );

#ifdef CHECK_TLS
#ifdef __x86_64__
      if( flags & CLONE_SETTLS ) {
	unsigned long fsaddr;
	arch_prctl( ARCH_GET_FS, &fsaddr );
	pip_print_maps();
	fprintf( stderr,
		 "TLS=%p  tls.base_addr=%u  FS=%p  PThread=%p  STACK=%p\n",
		 (void*) tls, ((struct user_desc *) tls)->base_addr,
		 (void*) fsaddr, (void*) pthread_self(), child_stack );
      } else {
	fprintf( stderr, "CLONE_SETTLS is not set\n" );
      }
#endif
#endif

      if( retval > 0 ) {	/* created PID is returned */
	pip_clone_info.flag_clone = flags;
	pip_clone_info.pid_clone  = retval;
	pip_clone_info.stack      = child_stack;
      }
    } else {
      DBGF( "!!! wrpper clone() ??????" );
    }
    va_end( ap );
  }
 error:
  pip_spin_unlock( &pip_clone_info.lock );
  return retval;
}
