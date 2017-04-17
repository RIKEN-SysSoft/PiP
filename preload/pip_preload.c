/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <dlfcn.h>
#include <sched.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>

#include <pip_clone.h>

//#define DEBUG
#include <pip.h>
#include <pip_debug.h>

//#define CHECK_TLS
#ifdef CHECK_TLS
#include <asm/prctl.h>
#include <asm/ldt.h>
#include <sys/prctl.h>
#include <pthread.h>
#endif

typedef
int(*clone_syscall_t)(int(*)(void*), void*, int, void*, pid_t*, void*, pid_t*);

pip_clone_t pip_clone_info = { 0 }; /* this is refered by piplib */

static clone_syscall_t pip_clone_orig = NULL;

static clone_syscall_t pip_get_clone( void ) {
  static clone_syscall_t pip_clone_orig = NULL;

  if( pip_clone_orig == NULL ) {
    pip_clone_orig = (clone_syscall_t) dlsym( RTLD_NEXT, "__clone" );
  }
  return pip_clone_orig;
}

int __clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  pip_spinlock_t oldval;
  int retval = -1;

  while( 1 ) {
    oldval = pip_spin_trylock_wv( &pip_clone_info.lock, PIP_LOCK_OTHERWISE );
    switch( oldval ) {
    case PIP_LOCK_UNLOCKED:
      /* lock succeeded */
    case PIP_LOCK_PRELOADED:
      /* locked by piplib */
      goto lock_ok;
      break;
    case PIP_LOCK_OTHERWISE:
    default:
      /* somebody other than piplib locked already */
      /* and wait until it is unlocked */
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
      retval = pip_clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );

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

    } else if( oldval == PIP_LOCK_PRELOADED ) {
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
      //flags |= CLONE_PTRACE;

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
