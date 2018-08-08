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

#define PIP_INTERNAL_FUNCS

#define NULPS	(NTASKS-10)
//#define NULPS	(10)
//#define NULPS	(3)

//#define NYIELDS	(5*1000)
//#define NYIELDS	(1*1000)
#define NYIELDS	(10)

//#define DEBUG
#define PIP_EVAL

#include <test.h>

pip_ulp_barrier_t barr;
pip_ulp_barrier_t *barrp = &barr;

int main( int argc, char **argv ) {
  int ntasks = 0;
  int nulps;
  int i, pipid;

  set_sigsegv_watcher();

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  }
  nulps = ntasks - 1;

  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_t ulps;

    pip_ulp_barrier_init( barrp, ntasks );
    PIP_ULP_INIT( &ulps );
    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
    }
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ) );

    for( i=0; i<nulps+1; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    double time_yield = 0.0;
    double one_yield;

    for( i=0; i<nulps*10; i++ ) { /* warming-up */
      TESTINT( pip_ulp_yield() );
    }

    pip_ulp_barrier_wait( barrp );
    for( i=0; i<NYIELDS; i++ ) {
      PIP_ACCUM( time_yield, pip_ulp_yield()==0 );
    }
    pip_ulp_barrier_wait( barrp );

    one_yield = time_yield;
    one_yield /= ((double) NYIELDS ) * ((double)ntasks);
    if( pip_is_task() ) {
      fprintf( stderr,
	       "Hello, this is TASK (%d)  "
	       "[one yield() call takes %g usec / %g sec]\n",
	       pipid,
	       one_yield*1e6, time_yield );
    } else if( pip_is_ulp() ) {
      fprintf( stderr,
	       "Hello, this is ULP (%d)  "
	       "[one yield() call takes %g usec / %g sec]\n",
	       pipid,
	       one_yield*1e6, time_yield );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
