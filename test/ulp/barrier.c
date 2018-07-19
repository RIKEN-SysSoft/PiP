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

//#define DEBUG

#include <test.h>
#include <pip_ulp.h>

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
#endif

#ifndef DEBUG
#define NULPS		(NTASKS-10)
#else
#define NULPS		(NTASKS-5)
#endif

#define BIAS		(10000)

struct expo {
  volatile int		c;
  pip_ulp_barrier_t 	barr;
} expo;

int main( int argc, char **argv ) {
  struct expo *expop;
  int ntasks = 0;
  int i, pipid, nulps;

  if( argc   > 1 ) {
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

  pip_ulp_barrier_init( &expo.barr, ntasks );
  expo.c = BIAS;
  expop  = &expo;
  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_queue_t ulps;

    PIP_ULP_INIT( &ulps );
    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
    }
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ));
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    if( pip_is_ulp() ) {

      /* Disturbances */
      int nyields = rand() % nulps;
      for( i=0; i<nyields; i++ ) TESTINT( pip_ulp_yield() );

      for( i=0; i<nulps; i++ ) {
	TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
	if( i == pipid ) {
	  if( (expop->c)++ == ( pipid + BIAS ) ) {
	    fprintf( stderr, "<%d> Hello ULP !!\n", pipid );
	  } else {
	    fprintf( stderr, "<%d> Bad ULP !!\n", pipid );
	  }
	}
      }
    } else {
      for( i=0; i<nulps; i++ ) {
	TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
      }
      if( pipid == ( ntasks - 1 ) && expop->c == ( pipid + BIAS ) ) {
	fprintf( stderr, "<%d> Hello TASK !!\n", pipid );
      } else {
	fprintf( stderr, "<%d> Bad TASK !!\n", pipid );
      }
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
