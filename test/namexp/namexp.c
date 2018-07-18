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
  int i, j, k, flag;

  set_signal_watcher( SIGSEGV );
  if( argc == 1 ) {
    nulps  = NULPS;
    ntasks = NTASKS / ( nulps + 1 );
  } else {
    if( argc > 1 ) ntasks = atoi( argv[1] );
    if( argc > 2 ) nulps  = atoi( argv[2] );
    if( argc > 3 ) niters = atoi( argv[3] );
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
  if( niters == 0 ) niters = NITERS;

  TESTINT( ( a = (int*) malloc( sizeof(int) * niters ) ) == NULL );
  for( i=0; i<niters; i++ ) a[i] = i + BIAS;

  barrp = &barr;
  TESTINT( pip_init( &pipid, &total, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_queue_t 	ulps;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    TESTINT( pip_universal_barrier_init( barrp, total ) );

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
    ntest = total / 9;
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
      if( pipid == 0 && i % 10 == 0 ) fprintf( stderr, "ITER:%d\n", i );
    }
#ifdef DEBUG
    fprintf( stderr, "\n[pipid:%d] DONE\n\n", pipid );
#endif
    pip_universal_barrier_wait( barrp );
  }
  return flag;
}
