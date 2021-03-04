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
 * System Software Development Team, 2016-2021
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info@googlegroups.com
 * User ML: procinproc-users@googlegroups.com
 * $
 */

#include <pip/pip_internal.h>

int pip_tgkill( int tgid, int tid, int signal ) {
  return (int) syscall( (long int) SYS_tgkill, tgid, tid, signal );
}

int pip_kill( int pipid, int signal ) {
  int err;
  if( pip_root == NULL            ) RETURN( EPERM  );
  if( signal < 0 || signal > _NSIG ) RETURN( EINVAL );
  if( ( err = pip_check_pipid( &pipid ) ) == 0 ) {
    err = pip_raise_signal( pip_get_task( pipid ), signal );
  }
  RETURN( err );
}

int pip_sigmask( int how, const sigset_t *sigmask, sigset_t *oldmask ) {
  int err;
  if( pip_is_threaded_() ) {
    err = pthread_sigmask( how, sigmask, oldmask );
  } else {
    errno = 0;
    (void) sigprocmask( how, sigmask, oldmask );
    err = errno;
  }
  return( err );
}

int pip_signal_wait( int signal ) {
  sigset_t 	sigset;
  int 		sig;
  ASSERT( sigemptyset( &sigset ) == 0 );
  ASSERT( sigaddset( &sigset, signal ) == 0 );
  return sigwait( &sigset, &sig );
}
