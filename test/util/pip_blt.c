/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

static pip_task_list_t	lists[NTASKS];

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  int 	nacts, npass, ntasks, pipid;
  char 	env_ntasks[128];
  char 	env_pipid[128];
  int 	i, j;

  set_sigsegv_watcher();

  if( argc < 4 ) return EXIT_UNTESTED;
  CHECK( access( argv[3], X_OK ),     RV, exit(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[3], 1 ), RV, exit(EXIT_UNTESTED) );

  nacts = strtol( argv[1], NULL, 10 );
  CHECK( nacts,  RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  npass = strtol( argv[2], NULL, 10 );
  CHECK( npass,  RV<0 ||RV>NTASKS, return(EXIT_UNTESTED) );
  ntasks = nacts + npass;
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );

  for( i=0; i<nacts; i++ ) PIP_LIST_INIT( &lists[i] );

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );

  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );
  pip_spawn_from_main( &prog, argv[3], &argv[3], NULL );

  for( i=0, j=0; i<npass; i++, j++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    j = ( j >= nacts ) ? 0 : j;
    CHECK( pip_blt_spawn( &prog, pipid, 0, PIP_TASK_PASSIVE,
			  NULL, &lists[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
  }
  for( i=npass, j=0; i<ntasks; i++, j++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_blt_spawn( &prog, pipid, PIP_CPUCORE_ASIS, 0,
			  NULL, &lists[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( PIP_LIST_ISEMPTY(&lists[j]), !RV, return(EXIT_FAIL) );
  }
  for( i=0; i<ntasks; i++ ) {
    int extval, status = 0;
    CHECK( pip_wait( i, &status ), RV, return(EXIT_UNTESTED) );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
      CHECK( extval, RV!=0, return(EXIT_FAIL) );
    } else {
      CHECK( "test program signaled", 1, return(EXIT_UNRESOLVED) );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_UNTESTED) );
  return 0;
}
