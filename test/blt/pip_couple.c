/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#define DEBUG
#include <test.h>

//#define NITERS		(100)
#define NITERS		(1)

typedef struct exp {
  pip_task_queue_t	queue;
  pip_barrier_t		barrier;
} exp_t;


int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	niters = NITERS;
  int	i, extval = 0;

  set_sigsegv_watcher();

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0      ) ? NTASKS-1 : ntasks;
  ntasks = ( ntasks >= NTASKS ) ? NTASKS-1 : ntasks;
  ntasks ++;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_task_queue_init(&expp->queue ,NULL), RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_init(&expp->barrier,ntasks), RV, return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, PIP_CPUCORE_ASIS, PIP_TASK_ACTIVE, &pipid,
			    NULL, NULL, NULL ),
	     RV,
	     abend(EXIT_UNTESTED) );
    }

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any(NULL,&status), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }
  } else {
    pid_t pid = getpid();
    if( pipid < ntasks-1 ) {
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	CHECK( pid==getpid(),            !RV, return(EXIT_FAIL) );
	CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
	CHECK( pid==getpid(),             RV, return(EXIT_FAIL) );
	CHECK( pip_yield(PIP_YIELD_USER), (RV&&RV!=EINTR), return(EXIT_FAIL) );
      }
    } else {
      pip_task_t *task;
      int 	 n = PIP_TASK_ALL;

      CHECK( pip_get_task_from_pipid(PIP_PIPID_MYSELF,&task), RV, 
	     return(EXIT_FAIL) );
      CHECK( pip_dequeue_and_resume_N(&expp->queue,task,&n), RV, 
	     return(EXIT_FAIL) );
      CHECK( n!=ntasks-1,                          RV, return(EXIT_FAIL) );
      do {} while( pip_yield(PIP_YIELD_USER) == EINTR );
    }
    CHECK( pip_barrier_wait(&expp->barrier), RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
