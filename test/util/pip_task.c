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
static int get_ncpus( void ) {
  cpu_set_t cpuset;
  CHECK( sched_getaffinity( 0, sizeof(cpuset), &cpuset ),
	 RV, exit(EXIT_UNTESTED) );
  return CPU_COUNT( &cpuset );
}
#endif

int main( int argc, char **argv ) {
  int ntasks, ntenv, pipid, status, extval;
  int i, c, nc, err;
  char env_ntasks[128], env_pipid[128];
  char *env;

  set_sigint_watcher();

  if( argc < 3 ) return EXIT_UNTESTED;
  CHECK( access( argv[2], X_OK ),     RV, return(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[2], 1 ), RV, return(EXIT_UNTESTED) );

  ntasks = strtol( argv[1], NULL, 10 );
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  }
  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_UNTESTED) );
  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );

  nc = 0;
#ifdef DO_COREBIND
  nc = get_ncpus() - 1;
#endif

  for( i=0; i<ntasks; i++ ) {
    pipid = i;
#ifndef DO_COREBIND
    c = PIP_CPUCORE_ASIS;
#else
    if( nc == 0 ) {
      c = PIP_CPUCORE_ASIS;
    } else {
      c = i % nc;
    }
#endif
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_spawn( argv[2], &argv[2], NULL, c, &pipid,
		      NULL, NULL, NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( pipid, RV!=i, return(EXIT_UNTESTED) );
  }
  err    = 0;
  extval = 0;
  for( i=0; i<ntasks; i++ ) {
    status = 0;
    CHECK( pip_wait( i, &status ), RV, return(EXIT_UNTESTED) );
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

  CHECK( pip_fin(), RV, return(EXIT_UNTESTED) );
  return err;
}
