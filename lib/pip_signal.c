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

int pip_tgkill( int tgid, int tid, int signal ) {
  return (int) syscall( (long int) SYS_tgkill, tgid, tid, signal );
}

int pip_tkill( int tid, int signal ) {
  return (int) syscall( (long int) SYS_tkill, tid, signal );
}

/* for internal use */
int pip_raise_signal( pip_task_internal_t *taski, int sig ) {
  int err = ESRCH;

  DBGF( "raise signal (%s) to PIPID:%d", strsignal(sig), TA(taski)->pipid );
  if( AA(taski)->flag_exit == 0 ) {
    if( TA(taski)->task_sched != taski &&
	TA(taski)->schedq_len > 0 ) {
      /* Not allowed to a signal to an inactive task */
      RETURN( EPERM );
    } else if( AA(taski)->tid > 0 ) {
      DBGF( "taski->annex->tid: %d", AA(taski)->tid );
      if( pip_is_threaded_() ) {
	err = pthread_kill( MA(taski)->thread, sig );
      } else {
	errno = 0;
	if( pip_tkill( AA(taski)->tid, sig ) ) {
	  RETURN( errno );
	}
	err = 0;
      }
    }
  }
  RETURN( err );
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
  ASSERTS( sigemptyset( &sigset ) );
  ASSERTS( sigaddset( &sigset, signal ) );
  return sigwait( &sigset, &sig );
}
