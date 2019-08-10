/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

static pip_task_queue_t	queues[NTASKS];

static pip_barrier_t	barr;

static int is_taskq_empty( pip_task_queue_t *queue ) {
  int rv;

  pip_task_queue_lock( queue );
  rv = pip_task_queue_isempty( queue );
  pip_task_queue_unlock( queue );
  return rv;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  pip_barrier_t		*barrp = &barr;
  int 	nacts, npass, ntasks, pipid;
  int	count;
  char 	env_ntasks[128];
  char 	env_pipid[128];
  int 	i, j;

  set_sigsegv_watcher();
  set_sigint_watcher();

  if( argc < 4 ) return EXIT_UNTESTED;
  CHECK( access( argv[3], X_OK ),     RV, exit(EXIT_UNTESTED) );
  CHECK( pip_check_pie( argv[3], 1 ), RV, exit(EXIT_UNTESTED) );

  nacts = strtol( argv[1], NULL, 10 );
  CHECK( nacts,  RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );
  npass = strtol( argv[2], NULL, 10 );
  CHECK( npass,  RV<0 ||RV>NTASKS, return(EXIT_UNTESTED) );
  ntasks = nacts + npass;
  CHECK( ntasks, RV<=0||RV>NTASKS, return(EXIT_UNTESTED) );

  for( i=0; i<nacts; i++ ) pip_task_queue_init( &queues[i], NULL );
  CHECK( pip_barrier_init( &barr, ntasks ), RV, return(EXIT_FAIL) );

  CHECK( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ), RV, return(EXIT_FAIL) );
  sprintf( env_ntasks, "%s=%d", PIP_TEST_NTASKS_ENV, ntasks );
  putenv( env_ntasks );
  pip_spawn_from_main( &prog, argv[3], &argv[3], NULL );

  count = 0;
  CHECK( pip_named_export( (void*) &count, "COUNT" ), RV, return(EXIT_FAIL) );

  for( i=0, j=0; i<npass; i++, j++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    j = ( j >= nacts ) ? 0 : j;
    CHECK( pip_blt_spawn( &prog, 0, PIP_TASK_PASSIVE, &pipid,
			  NULL, &queues[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( is_taskq_empty( &queues[j]), RV, return(EXIT_FAIL) );
  }
  for( i=npass, j=0; i<ntasks; i++, j++ ) {
    pipid = i;
    sprintf( env_pipid, "%s=%d", PIP_TEST_PIPID_ENV, pipid );
    putenv( env_pipid );
    CHECK( pip_blt_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid,
			  NULL, &queues[j], NULL ),
	   RV,
	   return(EXIT_UNTESTED) );
    CHECK( is_taskq_empty( &queues[j]), !RV, return(EXIT_FAIL) );
  }
  for( i=0; i<ntasks; i++ ) {
    int extval, status = 0;
    CHECK( pip_wait_any( &pipid, &status ), RV, return(EXIT_FAIL) );
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
