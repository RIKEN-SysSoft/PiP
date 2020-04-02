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

//#define DEBUG
#include <test.h>

static struct my_exp	*expp;
static int 		ntasks, pipid;

static struct my_exp {
  pthread_barrier_t	barr;
} exp;

static int block_sigusrs( void ) {
  sigset_t 	ss;
  errno = 0;
  if( sigemptyset( &ss )        != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR1 ) != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR2 ) != 0 ) return errno;
  return pip_sigmask( SIG_BLOCK, &ss, NULL );
}

static int wait_signal_root( void ) {
  sigset_t 	ss;
  int		sig;

  if( sigemptyset( &ss )        != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR1 ) != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR2 ) != 0 ) return errno;
  (void) sigwait( &ss, &sig );
  return sig;
}

static int wait_signal_task( int pipid ) {
  sigset_t 	ss;
  int 		sig, next = pipid + 1;

  if( sigemptyset( &ss )        != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR1 ) != 0 ) return errno;
  if( sigaddset( &ss, SIGUSR2 ) != 0 ) return errno;
  (void) sigwait( &ss, &sig );
  if( next >= ntasks ) next = PIP_PIPID_ROOT;
  CHECK( pip_kill( next, sig ), RV, exit(EXIT_FAIL) );
  return sig;
}

int main( int argc, char **argv ) {
  int	i, recv_sig, extval = 0;

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  expp = &exp;
  CHECK( pip_init(&pipid,&ntasks,(void**)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    memset( &exp, 0, sizeof(exp) );
    CHECK( pthread_barrier_init(&exp.barr,NULL,ntasks+1),
	   RV, return(EXIT_FAIL) );

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
		       NULL,NULL,NULL),
	     RV,
	     return(EXIT_FAIL) );
    }
    CHECK( block_sigusrs(), 		    RV, return(EXIT_FAIL) );
    CHECK( pthread_barrier_wait( &expp->barr ),
	   (RV!=0&&RV!=PTHREAD_BARRIER_SERIAL_THREAD), return(EXIT_FAIL) );

    CHECK( pip_kill( 0, SIGUSR1 ),          RV,    return(EXIT_FAIL) );
    CHECK( recv_sig = wait_signal_root(),   RV==0, return(EXIT_FAIL) );
    CHECK( recv_sig!=SIGUSR1, 		    RV,    return(EXIT_FAIL) );

    CHECK( pip_kill( 0, SIGUSR2 ),          RV,    return(EXIT_FAIL) );
    CHECK( recv_sig = wait_signal_root(),   RV==0, return(EXIT_FAIL) );
    CHECK( recv_sig!=SIGUSR2, 		    RV,    return(EXIT_FAIL) );

    CHECK( pthread_barrier_wait( &expp->barr ),
	   (RV!=0&&RV!=PTHREAD_BARRIER_SERIAL_THREAD), return(EXIT_FAIL) );

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
    CHECK( pip_kill( ntasks, SIGUSR1 ), RV!=ERANGE, return(EXIT_FAIL) );

  } else {
    CHECK( block_sigusrs(), RV, return(EXIT_FAIL) );

    CHECK( pthread_barrier_wait( &expp->barr ),
	   (RV!=0&&RV!=PTHREAD_BARRIER_SERIAL_THREAD), return(EXIT_FAIL) );

    CHECK( recv_sig = wait_signal_task(pipid),  RV==0, return(EXIT_FAIL) );
    CHECK( recv_sig==SIGUSR1,                  !RV,    return(EXIT_FAIL) );
    CHECK( recv_sig = wait_signal_task(pipid),  RV==0, return(EXIT_FAIL) );
    CHECK( recv_sig==SIGUSR2,                  !RV,    return(EXIT_FAIL) );

    CHECK( pthread_barrier_wait( &expp->barr ),
	   (RV!=0&&RV!=PTHREAD_BARRIER_SERIAL_THREAD), return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return extval;
}
