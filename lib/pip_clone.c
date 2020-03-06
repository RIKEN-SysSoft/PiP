/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <pip_internal.h>
#include <pip_dlfcn.h>
#include <stdarg.h>

typedef int(*pip_clone_syscall_t)(int(*)(void*), void*, int, void*, ...);

pip_clone_syscall_t	pip_clone_orig;
pip_spinlock_t 		pip_lock_got_clone PIP_PRIVATE;

static int pip_clone_flags( int flags ) {
  flags &= ~(CLONE_FS);	 /* 0x00200 */
  flags &= ~(CLONE_FILES);	 /* 0x00400 */
  flags &= ~(CLONE_SIGHAND); /* 0x00800 */
  flags &= ~(CLONE_THREAD);	 /* 0x10000 */
  flags &= ~0xff;
  flags |= CLONE_VM;
  flags |= CLONE_SETTLS;  /* do not reset the CLONE_SETTLS flag */
  flags |= CLONE_PTRACE;
  flags |= SIGCHLD;
  return flags;
}

static pip_spinlock_t pip_lock_clone( pid_t tid ) {
  pip_spinlock_t oldval;

  while( 1 ) {
    oldval = pip_spin_trylock_wv( &pip_lock_got_clone, PIP_LOCK_OTHERWISE );
    if( oldval == tid ) {
      /* called and locked by PiP lib */
      break;
    }
    if( oldval == PIP_LOCK_UNLOCKED ) { /* lock succeeds */
      /* not called by PiP lib */
      break;
    }
  }
  return oldval;
}

static int
pip_clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  pid_t		 tid = pip_gettid();
  pip_spinlock_t oldval;
  int 		 retval = -1;

  ENTER;
  oldval = pip_lock_clone( tid );
  do {
    va_list ap;
    va_start( ap, args );
    pid_t *ptid = va_arg( ap, pid_t* );
    void  *tls  = va_arg( ap, void*  );
    pid_t *ctid = va_arg( ap, pid_t* );

    if( oldval == tid ) {
      flags = pip_clone_flags( flags );
    }
    retval = pip_clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
    va_end( ap );
  } while( 0 );
  pip_spin_unlock( &pip_lock_got_clone );
  DBGF( "<< retval:%d", retval );
  return retval;
}

int pip_wrap_clone( void ) {
  ENTER;
  pip_clone_orig = pip_dlsym( RTLD_DEFAULT, "__clone" );
  if( pip_clone_orig == NULL ) RETURN( ENOSYS );
  RETURN( pip_patch_GOT( "libpthread.so", "__clone", pip_clone ) );

}
