/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

//#define DEBUG

#include <test.h>
#include <pip_ulp.h>
#include <pip_universal.h>

#define NITERS	(10000)
//#define NITERS	(10)
#define NULPS	(10)

struct expo {
  pip_barrier_t			pbarr;
  pip_universal_barrier_t	ubarr;
  int				*count;
  int				error;
} expo;

int main( int argc, char **argv, char **envv ) {
  struct expo *expop = &expo;
  int pipid, nulps=0, ntasks=0, total;
  int niters=0, i, j, k;

  if( argc == 1 ) {
    nulps  = NULPS;
    ntasks = NTASKS / ( nulps + 1 );
  } else {
    if( argc > 1 ) ntasks = atoi( argv[1] );
    if( argc > 2 ) nulps  = atoi( argv[2] );
    if( argc > 3 ) niters = atoi( argv[3] );
  }
  if( ntasks == 0 ) {
      fprintf( stderr, "Not enough number of tasks\n" );
      exit( 1 );
  }
  total = ntasks * nulps + ntasks;
  if( total > NTASKS ) {
    fprintf( stderr, "Too large\n" );
    exit( 1 );
  }
  if( niters == 0 ) niters = NITERS;

  TESTINT( pip_init( &pipid, &total, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_queue_t 	ulps;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    TESTINT( pip_barrier_init(           &expop->pbarr, ntasks ) );
    TESTINT( pip_universal_barrier_init( &expop->ubarr, total  ) );
    expop->count = (int*) malloc( sizeof(int) * total );
    expop->error = 0;
    for( i=0; i<total; i++ ) expop->count[i] = 0;
    k = 0;
    for( i=0; i<ntasks; i++ ) {
      PIP_LIST_INIT( &ulps );
      for( j=0; j<nulps; j++ ) {
	pipid = k++;
	TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
      }
      pipid = k++;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ));
    }
    for( i=0; i<total; i++ ) {
      TESTINT( pip_wait_any( NULL, NULL ) );
    }
    TESTINT( pip_universal_barrier_fin( &expop->ubarr ) );
    if( expop->error ) {
      fprintf( stderr, "failure\n" );
    } else {
      fprintf( stderr, "success\n" );
    }
  } else {
    for( i=0; i<nulps; i++ ) {
      TESTINT( pip_ulp_yield() );
    }
    TESTINT( pip_barrier_wait( &expop->pbarr ) );

    for( i=0; i<niters; i++ ) {
      TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
      if( pipid == 0 ) {
	int j;
	for( j=0; j<total; j++ ) {
	  if( expop->count[j] != i ) {
	    fprintf( stderr, "[%d] count[%d]:%d != %d\n",
		     pipid, j, expop->count[j], i );
	    expop->error = 1;
	  }
	}
	if( isatty(1) ) fprintf( stderr, "[%d] %d\n", pipid, i );
      }
      TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
      if( rand() % total == pipid ) usleep( 100 ); /* disturbance */
      //if( rand() % total == pipid ) sleep( 5 );
      expop->count[pipid] ++;
      TESTINT( pip_universal_barrier_wait( &expop->ubarr ) );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
