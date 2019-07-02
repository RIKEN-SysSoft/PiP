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

static struct my_exp *expp;
static int ntasks, pipid;
static volatile int recv_sig;

static struct my_exp {
  pip_barrier_t		barr;
} exp;

static int block_sigusrs( void ) {
  sigset_t 	sigset;
  int 		flag, err;

  errno = 0;
  if( sigemptyset( &sigset )             != 0 ) return errno;
  if( sigaddset( &sigset, SIGUSR1 )      != 0 ) return errno;
  if( sigaddset( &sigset, SIGUSR2 )      != 0 ) return errno;
  if( ( err = pip_is_threaded( &flag ) ) != 0 ) return err;
  return pip_sigmask( SIG_BLOCK, &sigset, NULL );
}

static int wait_signal( void ) {
  sigset_t 	sigset;
  int 		flag, err;

  if( sigemptyset( &sigset )                              != 0 ) return errno;
  if( ( err = pip_sigmask( SIG_UNBLOCK, &sigset, NULL ) ) != 0 ) return err;
  if( ( err = pip_is_threaded( &flag )                  ) != 0 ) return err;
  if( !flag ) {
    (void) sigsuspend( &sigset ); /* always returns EINTR */
    return 0;
  } else {
    while( recv_sig == 0 ) pause_and_yield(0);
    recv_sig = 0;
  }
  return 0;
}

static void sigusr_root( int sig ) {
#ifdef DEBUG
  fprintf( stderr, "[ROOT] Signal(%d) received\n", sig );
#endif
  recv_sig = sig;
}

static void sigusr_task( int sig ) {
  int next = pipid + 1;
#ifdef DEBUG
  fprintf( stderr, "[%d] Signal(%d) received\n", pipid, sig );
#endif
  recv_sig = sig;
  if( next == ntasks ) next = PIP_PIPID_ROOT;
  CHECK( pip_kill( next, sig ), RV, exit(EXIT_FAIL) );
}

int main( int argc, char **argv ) {
  int	i, extval = 0;

  set_sigsegv_watcher();

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  expp = &exp;;
  CHECK( pip_init(&pipid,&ntasks,(void**)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&exp.barr,ntasks+1),
	   RV, return(EXIT_FAIL) );
    CHECK( set_signal_handler( SIGUSR1, sigusr_root ),
	   RV, return(EXIT_UNTESTED) );
    CHECK( set_signal_handler( SIGUSR2, sigusr_root ),
	   RV, return(EXIT_UNTESTED) );

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
		       NULL,NULL,NULL),
	     RV,
	     return(EXIT_FAIL) );
    }
    CHECK( block_sigusrs(), 		    RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_wait( &expp->barr ), RV, return(EXIT_FAIL) );

    CHECK( pip_kill( 0, SIGUSR1 ),          RV, return(EXIT_FAIL) );
    CHECK( wait_signal(), 	            RV, return(EXIT_FAIL) );
    CHECK( recv_sig!=SIGUSR1, 		    RV, return(EXIT_FAIL) );
    recv_sig = 0;

    CHECK( pip_kill( 0, SIGUSR2 ), 	    RV, return(EXIT_FAIL) );
    CHECK( wait_signal(), 	            RV, return(EXIT_FAIL) );
    CHECK( recv_sig!=SIGUSR2, 		    RV, return(EXIT_FAIL) );
    recv_sig = 0;

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any( NULL, &status ), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }

  } else {
    CHECK( set_signal_handler( SIGUSR1, sigusr_task ),
	   RV, return(EXIT_UNTESTED) );
    CHECK( set_signal_handler( SIGUSR2, sigusr_task ),
	   RV, return(EXIT_UNTESTED) );
    CHECK( block_sigusrs(), 		    RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_wait( &expp->barr ), RV, return(EXIT_FAIL) );

    CHECK( wait_signal(), RV, return(EXIT_FAIL) );
    CHECK( wait_signal(), RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return extval;
}
