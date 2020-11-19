/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#include <pip/pip_internal.h>
#include <pip/pip_clone.h>

#include <sys/syscall.h>
#include <dlfcn.h>

//#define DEBUG

pip_clone_t pip_clone_info = { 0 }; /* refered by piplib */

static clone_syscall_t pip_clone_orig;

static pid_t pip_gettid_preloaded( void ) {
  return (pid_t) syscall( (long int) SYS_gettid );
}

static clone_syscall_t pip_get_clone( void ) {
  if( pip_clone_orig == NULL ) {
    pip_clone_orig = (clone_syscall_t) dlsym( RTLD_NEXT, "__clone" );
  }
  return pip_clone_orig;
}

int __clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  pid_t		 tid = pip_gettid_preloaded();
  pip_spinlock_t oldval;
  int 		 retval = -1;

  oldval = pip_clone_lock( tid, &pip_clone_info.lock );
  do {
    va_list ap;
    va_start( ap, args );
    pid_t *ptid = va_arg( ap, pid_t*);
    void  *tls  = va_arg( ap, void*);
    pid_t *ctid = va_arg( ap, pid_t*);

    if( pip_clone_orig == NULL ) {
      if( ( pip_clone_orig = pip_get_clone() ) == NULL ) {
	errno = ENOSYS;
	goto error;
      }
    }
    if( oldval == tid ) {
      flags = pip_clone_flags( flags );
    }
    errno = 0;
    retval = pip_clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
    va_end( ap );
  } while( 0 );
 error:
  pip_clone_unlock( &pip_clone_info.lock );
  return retval;
}
