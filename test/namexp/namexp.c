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

#define NITERS	(10)

pip_barrier_t barr;
int a;

int main( int argc, char **argv, char **envv ) {
  pip_barrier_t *barrp;
  int *ap;
  int pipid;
  int ntasks = 0;
  int i, flag;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  }
  if( ntasks < 2 || ntasks == 0 ) ntasks = NTASKS;

  barrp = &barr;
  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_barrier_init( barrp, ntasks );
    TESTINT( pip_named_export( (void*) barrp, "barr" ) );

    for( i=0; i<ntasks; i++ ) {
      int err;
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
    }
    for( i=0; i<ntasks; i++ ) {
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
    int next = ( pipid + 1 ) % ntasks;
    int n;

    pip_barrier_wait( barrp );
    for( i=0; i<NITERS; i++ ) {
      DBGF( "barrp:%p", barrp );
      a = pipid * 10000 + i;
      ap = &a;
      TESTINT( pip_named_export( (void*) ap, "%d:a", a ) );
      n = next  * 10000 + i;
      TESTINT( pip_named_import( next, (void**) &ap, "%d:a", n ) );
      if( *ap != n ) {
	flag = 1;
	fprintf( stderr, "[%d] i=%d  %d != %d\n", pipid, i, *ap, n );
      }
    }
  }
  DBG;
  return flag;
}
