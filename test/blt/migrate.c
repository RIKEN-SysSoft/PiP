/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG
#include <test.h>

#define NITERS		(100)

static pip_task_queue_t	queue[2], *queuep[2];

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  pip_task_queue_t	**qpp;
  int 	ntasks, pipid;
  int	niters;
  int	i, j, k, l, c, extval = 0;

  set_sigsegv_watcher();

  CHECK( pip_task_queue_init(&queue[0],NULL), RV, return(EXIT_FAIL) );
  CHECK( pip_task_queue_init(&queue[1],NULL), RV, return(EXIT_FAIL) );
  pip_spawn_from_main( &prog, argv[0], argv, NULL );

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  niters = 0;
  if( argc > 2 ) {
    niters = strtol( argv[2], NULL, 10 );
  }
  niters = ( niters == 0 ) ? NITERS : niters;

  queuep[0] = &queue[0];
  queuep[1] = &queue[1];
  qpp = queuep;
  CHECK( pip_init(&pipid,&ntasks,(void*)&qpp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_task_t	*task;
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, 0, PIP_TASK_PASSIVE, &pipid,
			    NULL, qpp[0], NULL ),
	     RV,
	     abend(EXIT_UNTESTED) );
    }
    l = 1;
    for( i=0; i<niters+1; i++ ) {
      l ^= 0x01;
      while( 1 ) {
	CHECK( pip_task_queue_count(qpp[l],&c),     RV, return(EXIT_FAIL) );
	if( c == ntasks ) break;
	CHECK( pip_yield(PIP_YIELD_SYSTEM),         RV, return(EXIT_FAIL) );
	usleep( 1000 );	/* 1 ms */
      } 
      for( j=0; j<ntasks; j++ ) {
	k = ( j + i ) % ntasks;
	CHECK( pip_get_task_from_pipid(k,&task),    RV, return(EXIT_FAIL) );
	CHECK( pip_dequeue_and_resume(qpp[l],task), RV, return(EXIT_FAIL) );
      }
      CHECK( pip_task_queue_isempty(qpp[l]),       !RV, return(EXIT_FAIL) );
    }

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any(NULL,&status),            RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }
  } else {
    l = 0;
    for( i=0; i<niters; i++ ) {
      l ^= 0x01;
      CHECK( pip_suspend_and_enqueue(qpp[l],NULL,NULL), RV, return(EXIT_FAIL) );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
