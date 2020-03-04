/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG
#include <test.h>

#define DO_COREBIND

#ifdef DO_COREBIND
extern int pip_do_corebind( int, cpu_set_t* );

static int get_ncpus( void ) {
  cpu_set_t cpuset;
  CHECK( sched_getaffinity( 0, sizeof(cpuset), &cpuset ),
	 RV, exit(EXIT_UNTESTED) );
  return CPU_COUNT( &cpuset );
}
#endif

int main( int argc, char **argv ) {
  int ntasks, pipid, status, extval;
  int i, c, nc, err;
  char env_ntasks[128];
  char env_pipid[128];

  set_sigsegv_watcher();
  set_sigint_watcher();

  if( argc < 3 ) return EXIT_UNTESTED;
  CHECK( access( argv[2], X_OK ),     RV, abend(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[2], 1 ), RV, abend(EXIT_UNTESTED) );

  ntasks = strtol( argv[1], NULL, 10 );
  CHECK( ntasks, RV<=0||RV>NTASKS, abend(EXIT_UNTESTED) );
  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, abend(EXIT_UNTESTED) );
  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );

#ifdef DO_COREBIND
  nc = get_ncpus() - 1;
  CHECK( pip_do_corebind(0,NULL), RV, abend(EXIT_UNTESTED) );
#endif

  for( i=0; i<ntasks; i++ ) {
    pipid = i;
#ifndef DO_COREBIND
    c = PIP_CPUCORE_ASIS;
#else
    if( nc == 0 ) {
      c = PIP_CPUCORE_ASIS;
    } else {
      c = ( i % nc ) + 1;
    }
#endif
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_spawn( argv[2], &argv[2], NULL, c, &pipid,
		      NULL, NULL, NULL ),
	   RV,
	   abend(EXIT_UNTESTED) );
    CHECK( pipid, RV!=i, return(EXIT_UNTESTED) );
  }
  err    = 0;
  extval = 0;
  for( i=0; i<ntasks; i++ ) {
    status = 0;
    CHECK( pip_wait( i, &status ), RV, abend(EXIT_UNTESTED) );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      if( extval ) {
	fprintf( stderr, "Task[%d] returns %d\n", i, extval );
      }
    } else {
      extval = EXIT_UNRESOLVED;
    }
    if( err == 0 && extval != 0 ) err = extval;
  }

  CHECK( pip_fin(), RV, abend(EXIT_UNTESTED) );
  return err;
}