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
#include <time.h>

pip_barrier_t barr, *barrp;

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

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks = 0;
  int extval;
  int flag, i;

  if( argc   > 1 ) ntasks = atoi( argv[1] );
  if( ntasks < 1 ) ntasks = NTASKS;
  ntasks = ( ntasks > 256 ) ? 256 : ntasks;

  barrp = &barr;
  pip_barrier_init( barrp, ntasks+1 );

  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			  NULL, NULL, NULL ) );
    }
    pip_barrier_wait( barrp );
    my_sleep( ntasks / 2 );
    flag = 0;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait_any( &pipid, &extval ) );
      if( pipid != extval ) {
	flag = 1;
	fprintf( stderr, "NGNGNG [ROOT] PIPID[%d] %d\n", pipid, extval );
      } else {
#ifdef DEBUG
	fprintf( stderr, "OKOKOK [ROOT] PIPID[%d] %d\n", pipid, extval );
#endif
      }
    }
    if( !flag ) {
      fprintf( stderr, "SUCCEEDED\n" );
    } else {
      fprintf( stderr, "FAILED\n" );
    }
    TESTINT( pip_fin() );

  } else {
    pip_barrier_wait( barrp );
    if( pipid > ntasks/2 ) my_sleep( ntasks - pipid );
    //fprintf( stderr, "[%d] Hello, I am fine !!\n", pipid );
    pip_exit( pipid );
    /* never reach here */
  }
  return 0;
}
