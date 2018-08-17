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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>

pip_task_barrier_t barr;

int main( int argc, char **argv ) {
  pip_task_barrier_t *barrp = &barr;
  int pipid = 999;
  int ntasks;
  int extval = 0;

  ntasks = 2;
  TESTINT( pip_init( &pipid, &ntasks, (void**)&barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    int st0, st1;

    TESTINT( pip_task_barrier_init( barrp, 3 ) );
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    pipid = 1;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );

    TESTINT( pip_task_barrier_wait( barrp ) );
    TESTINT( pip_kill( 0, SIGUSR1 ) );
    TESTINT( pip_kill( 1, SIGUSR2 ) );
    TESTINT( pip_wait( 0, &st0 ) );
    TESTINT( pip_wait( 1, &st1 ) );
    //fprintf( stderr, "st0=%d  st1=%d\n", st0, st1 );
    if( st0 == SIGUSR1 && st1 == SIGUSR2 ) {
      printf( "OK\n" );
    } else {
      printf( "NG\n" );
    }
  } else {
    void sigusr01_handler( int sig ) { fprintf( stderr, "[0] SIGUSR1\n" ); }
    void sigusr02_handler( int sig ) { fprintf( stderr, "[0] SIGUSR2\n" ); }
    void sigusr11_handler( int sig ) { fprintf( stderr, "[1] SIGUSR1\n" ); }
    void sigusr12_handler( int sig ) { fprintf( stderr, "[1] SIGUSR2\n" ); }
    sigset_t sigset;
    if( pipid == 0 ) {
      set_signal_handler( SIGUSR1, sigusr01_handler );
      set_signal_handler( SIGUSR2, sigusr02_handler );
      (void) sigfillset( &sigset );
      sigprocmask( SIG_BLOCK, &sigset, NULL );
      (void) sigfillset( &sigset );
      (void) sigdelset( &sigset, SIGUSR1 );
      TESTINT( pip_task_barrier_wait( barrp ) );
      (void) sigsuspend( &sigset );
      extval = SIGUSR1;
    } else {
      set_signal_handler( SIGUSR1, sigusr11_handler );
      set_signal_handler( SIGUSR2, sigusr12_handler );
      (void) sigfillset( &sigset );
      sigprocmask( SIG_BLOCK, &sigset, NULL );
      (void) sigfillset( &sigset );
      (void) sigdelset( &sigset, SIGUSR2 );
      TESTINT( pip_task_barrier_wait( barrp ) );
      (void) sigsuspend( &sigset );
      extval = SIGUSR2;
    }
  }
  TESTINT( pip_fin() );
  return extval;
}
