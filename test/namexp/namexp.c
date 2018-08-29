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
#include <math.h>

//#define NITERS	(10000)
#define NITERS	(100)

#define NULPS	(10)

#define BIAS	(2000)

pip_universal_barrier_t barr;
int *a;

int main( int argc, char **argv, char **envv ) {
  pip_universal_barrier_t *barrp;
  int *ap;
  int pipid;
  int ntasks=0, nulps=0, niters=0, ntest, total;
  int i, flag;

  set_signal_watcher( SIGSEGV );
  if( argc > 2 ) {
    nulps  = atoi( argv[2] );
  } else {
    nulps  = NULPS;
  }
  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS / ( nulps + 1 );
  }
  if( argc > 3 ) {
    niters = atoi( argv[3] );
  } else {
    niters = NITERS;
  }

  total = ntasks * nulps + ntasks;
  if( total < 1 ) {
      fprintf( stderr, "Not enough number of tasks\n" );
      exit( 1 );
  }
  if( total > NTASKS ) {
    fprintf( stderr, "Too large\n" );
    exit( 1 );
  }

  TESTINT( ( a = (int*) malloc( sizeof(int) * niters ) ) == NULL );
  for( i=0; i<niters; i++ ) a[i] = i + BIAS;

  barrp = &barr;
  TESTINT( pip_init( &pipid, &total, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    TESTINT( pip_universal_barrier_init( barrp, total ) );

    k = 0;
    for( i=0; i<ntasks; i++ ) {
      for( j=0; j<nulps; j++ ) {
	pipid = k++;
	TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
      }
      pipid = k++;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ));
    }
    for( i=0; i<total; i++ ) {
      int status;
      TESTINT( pip_wait_any( NULL, &status ) );
      if( status != 0 ) flag = 1;
    }
    TESTINT( pip_fin() );
    if( flag ) {
      fprintf( stderr, "success\n" );
    } else {
      fprintf( stderr, "FAILED\n" );
    }
  } else {
#ifdef DEBUG
    for( i=0; i<nulps; i++ ) pip_ulp_yield();
    TESTINT( pip_universal_barrier_wait( barrp ) );
    if( pipid == 0 ) fprintf( stderr, "\nGO\n\n" );
#endif
    ntest = (int) sqrtf( (float) total );
    if( ntest == 0 ) ntest = 5;
    srand( pipid );
    TESTINT( pip_universal_barrier_wait( barrp  ) );
    for( i=0; i<niters; i++ ) {
      ap = &a[i];
      TESTINT( pip_named_export( (void*) ap, "a[%d]", i ) );
      for( j=0; j<ntest; j++ ) {
	k = rand() % total;
	TESTINT( pip_named_import( k, (void**) &ap, "a[%d]", i ) );
	if( *ap != i+BIAS ) {
	  flag = 1;
	  fprintf( stderr, "[%d] i=%d  %d != %d\n", pipid, i, *ap, i+BIAS );
	}
#ifdef DEBUG
	if( pipid == 0 ) fprintf( stderr, "ITER:%d : %d\n", i, j );
#endif
      }
      if( pipid == 0 && i % 10 == 0 ) {
	fprintf( stderr, "ITER:%d\n", i );
      }
    }
#ifdef DEBUG
    fprintf( stderr, "\n[pipid:%d] DONE\n\n", pipid );
#endif
    TESTINT( pip_universal_barrier_wait( barrp ) );
  }
  return flag;
}
