/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#define DEBUG
#include <test.h>

int main( int argc, char **argv ) {
  int ntasks, pipid, status, extval;
  int i, err;
  char env_ntasks[128];
  char env_pipid[128];

  set_sigsegv_watcher();

  if( argc < 3 ) return EXIT_UNTESTED;
  CHECK( access( argv[2], X_OK ),     RV, exit(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[2], 1 ), RV, exit(EXIT_UNTESTED) );

  ntasks = strtol( argv[1], NULL, 10 );
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_UNTESTED) );
  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );

  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_spawn( argv[2], &argv[2], NULL, PIP_CPUCORE_ASIS, &pipid,
		      NULL, NULL, NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( pipid, RV!=i, return(EXIT_UNTESTED) );
  }
  extval = 0;
  err = 0;
  for( i=0; i<ntasks; i++ ) {
    status = 0;
    CHECK( pip_wait( i, &status ), RV, return(EXIT_UNTESTED) );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      CHECK( extval, RV!=0, RV=0 /*nop*/ );
    } else {
      CHECK( "test program signaled", 1, return(EXIT_UNRESOLVED) );
    }
    if( err == 0 && extval != 0 ) err = extval;
  }

  CHECK( pip_fin(), RV, return(EXIT_UNTESTED) );

  return extval;
}
