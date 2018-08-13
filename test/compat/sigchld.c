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

#include <signal.h>

//#define DEBUG
#include <test.h>
#include <time.h>

void my_sleep( int n ) {
  struct timespec tm, tr;
  tm.tv_sec = 0;
  tm.tv_nsec = n * 1000 * 1000;
#ifdef DEBUG
  tm.tv_sec  = 1 * n;
  tm.tv_nsec = 0;
#endif
  (void) nanosleep( &tm, &tr );
}

int count_sigchld = 0;;

void sigchld_handler( int sig ) {
#ifdef DEBUG
  fprintf( stderr, "SIGCHLD\n" );
#endif
  count_sigchld ++;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks = 0;
  int i;

  set_signal_watcher( SIGSEGV );

  if( argc   > 1 ) ntasks = atoi( argv[1] );
  if( ntasks < 1 ) ntasks = NTASKS;
  ntasks = ( ntasks > 256 ) ? 256 : ntasks;

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    struct sigaction 	sigact;

    memset( &sigact, 0, sizeof( sigact ) );
    TESTINT( sigemptyset( &sigact.sa_mask ) );
    TESTINT( sigaddset( &sigact.sa_mask, SIGCHLD ) );
    //sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = (void(*)()) sigchld_handler;
    TESTINT( sigaction( SIGCHLD, &sigact, NULL ) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			  NULL, NULL, NULL ) );
      TESTINT( pip_wait_any( NULL, NULL ) );
    }
    if( count_sigchld == ntasks ) {
      fprintf( stderr, "SUCCEEDED\n" );
    } else {
      fprintf( stderr, "FAILED (%d!=%d)\n", count_sigchld, ntasks );
    }
    TESTINT( pip_fin() );
  } else {
    pip_exit( pipid );
    /* never reach here */
  }
  return 0;
}
