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

#include <test.h>

#define NTHREADS	(4)
#define NITERS		(100)

static struct my_exp {
  pthread_barrier_t	barr;
} exp;

static struct my_exp 	*expp;
static int		niters;

void *thread_main( void *argp ) {
  struct my_exp *expp = (struct my_exp*) argp;
  int i;

  for( i=0; i<niters; i++ ) {
    CHECK( pthread_barrier_wait( &expp->barr ),
	   ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	   exit(EXIT_FAIL) );
  }
  return NULL;
}

int main( int argc, char **argv ) {
  pthread_t threads[NTHREADS];
  int 	ntasks, pipid, nthreads;
  int	i, extval = 0;

  set_sigsegv_watcher();

  ntasks = 0;
  if( argc > 1 ) ntasks = strtol( argv[1], NULL, 10 );
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  nthreads = 0;
  if( argc > 2 ) nthreads = strtol( argv[1], NULL, 10 );
  nthreads = ( nthreads == 0       ) ? NTHREADS : nthreads;
  nthreads = ( nthreads > NTHREADS ) ? NTHREADS : nthreads;

  niters = 0;
  if( argc > 3 ) niters = strtol( argv[3], NULL, 10 );
  niters = ( niters == 0 ) ? NITERS : niters;

  expp = &exp;
  CHECK( pip_init(&pipid,&ntasks,(void**)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pthread_barrier_init( &expp->barr, NULL, ntasks*nthreads ),
	   RV,
	   return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
		       NULL,NULL,NULL),
	     RV,
	     return(EXIT_FAIL) );
    }
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
    for( i=0; i<nthreads; i++ ) {
      CHECK( pthread_create( &threads[i], NULL, thread_main, expp ),
	     RV, return(EXIT_FAIL) );
    }
    for( i=0; i<nthreads; i++ ) {
      CHECK( pthread_join( threads[i], NULL ), RV, return(EXIT_FAIL) );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return extval;
}
