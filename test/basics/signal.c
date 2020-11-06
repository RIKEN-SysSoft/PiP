/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 1.2.0$
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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>

int send_wait_signal( int pipid, int ssig ) {
  sigset_t sigset;
  int wsig;
  int err;

  DBGF( "Root sends %s", signal_name( ssig ) );
  if( ( err = pip_kill( pipid, ssig ) ) == 0 ) {
    (void) sigfillset( &sigset );
    if( sigwait( &sigset, &wsig ) != 0 ) {
      err = errno;
    } else if( wsig != ssig ) {
      fprintf( stderr, "send_wait: SIGANL %d!=%d\n", ssig, wsig );
      err = ENXIO;
    } else {
      DBGF( "Root received %s", signal_name( wsig ) );
    }
  }
  RETURN( err );
}

int wait_send_signal( void ) {
  sigset_t sigset;
  int sig;
  int err;

  (void) sigfillset( &sigset );
  if( sigwait( &sigset, &sig ) != 0 ) {
    err = errno;
  } else if( ( err = pip_kill( PIP_PIPID_ROOT, sig ) ) != 0 ) {
    err = errno;
  }
  RETURN( err );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  volatile int flag = 0;
  void *exp;

  ntasks = 1;
  exp = (void*) &flag;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    watch_anysignal();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );

    while( flag == 0 ) pause_and_yield( 0 );

    TESTINT( pip_kill( pipid, SIGHUP  ) );
    TESTINT( pip_kill( pipid, SIGINT  ) );
    TESTINT( pip_kill( pipid, SIGQUIT ) );
    TESTINT( pip_kill( pipid, SIGILL  ) );
    TESTINT( pip_kill( pipid, SIGABRT ) );
    TESTINT( pip_kill( pipid, SIGFPE  ) );
    TESTINT( pip_kill( pipid, SIGSEGV ) );
    TESTINT( pip_kill( pipid, SIGPIPE ) );
    TESTINT( pip_kill( pipid, SIGALRM ) );
    TESTINT( pip_kill( pipid, SIGTERM ) );
    TESTINT( pip_kill( pipid, SIGUSR1 ) );
    TESTINT( pip_kill( pipid, SIGUSR2 ) );
    TESTINT( pip_kill( pipid, SIGCHLD ) );
    TESTINT( pip_kill( pipid, SIGTSTP ) );
    TESTINT( pip_kill( pipid, SIGTTIN ) );
    TESTINT( pip_kill( pipid, SIGTTOU ) );
    //TESTINT( pip_kill( pipid, SIGSTOP ) );
    TESTINT( pip_kill( pipid, SIGCONT ) );
    //TESTINT( pip_kill( pipid, SIGKILL ) );

    flag = 1;
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    echo_anysignal();

    *(int*)exp = 1;

    while( *(int*)exp == 1 ) pause_and_yield( 0 );
  }
  return 0;
}
