/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

static pip_task_t	*queue;

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  int 	nacts, npass, ntasks, pipid, status, extval;
  char 	env_ntasks[128];
  char 	env_pipid[128];
  int 	i, j, err;

  if( argc < 4 ) return EXIT_UNTESTED;
  CHECK( access( argv[2], X_OK ),     RV, exit(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[2], 1 ), RV, exit(EXIT_UNTESTED) );

  nacts = strtol( argv[1], NULL, 10 );
  CHECK( nacts,  RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  npass = strtol( argv[2], NULL, 10 );
  CHECK( npass,  RV<0 ||RV>NTASKS, return(EXIT_UNTESTED) );
  ntasks = nacts + npass;
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );

  queue = (pip_task_t*) malloc( sizeof(pip_task_t) * nacts );
  CHECK( (queue==NULL), RV, return(EXIT_UNTESTED) );
  for( i=0; i<nacts; i++ ) PIP_LIST_INIT( &queue[i] );

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );

  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );
  pip_spawn_from_main( &prog, argv[3], &argv[3], NULL );

  for( i=0, j=0; i<npass; i++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_blt_spawn( &prog, 0, pipid, PIP_TASK_PASSIVE,
			  NULL, NULL, &queue[j] ),
	   RV,
	   return(EXIT_UNTESTED) );
    j = ( ++j >= nacts ) ? 0 : nacts;
  }
  for( i=npass; i<ntasks; i++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_blt_spawn( &prog, PIP_CPUCORE_ASIS, pipid, 0,
			  NULL, NULL, &queue[i] ),
	   RV,
	   return(EXIT_UNTESTED) );
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
