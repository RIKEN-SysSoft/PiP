/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <test.h>

#define WITERS		(10)
#define NITERS		(1000)

typedef struct exp {
  pip_task_queue_t	queue;
  volatile int		done;
} exp_t;


int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	status, i, n, extval = 0;
  pid_t	pid;
  double t0, t1;

  ntasks = 2;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_task_queue_init(&expp->queue ,NULL), RV, return(EXIT_FAIL) );
    expp->done = 0;

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, i, PIP_TASK_ACTIVE, &pipid,
			    NULL, NULL, NULL ),
	     RV,
	     abend(EXIT_UNTESTED) );
    }
    for( i=0; i<ntasks; i++ ) {
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
    if( pipid == 0 ) {
      for( i=0; i<witers; i++ ) {
	t0 = pip_gettime();
	pid = getpid();
      }
      t0 = - pip_gettime();
      for( i=0; i<niters; i++ ) {
	pid = getpid();
      }
      t0 += pip_gettime();

      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      for( i=0; i<witers; i++ ) {
	t1 = pip_gettime();
	CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	CHECK( pid==getpid(),            !RV, return(EXIT_FAIL) );
	CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
      }
      expp->done ++;
      t1 = - pip_gettime();
      for( i=0; i<niters; i++ ) {
	CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	CHECK( pid==getpid(),            !RV, return(EXIT_FAIL) );
	CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
      }
      expp->done ++;
      t1 += pip_gettime();
      printf( "t0: %g\n", t0 / ((double) niters) );
      printf( "t1: %g\n", t1 / ((double) niters) );
    } else {			/* pipid ==2 */
      pip_task_t *task;
      CHECK( pip_get_task_from_pipid(PIP_PIPID_MYSELF,&task), RV,
	     return(EXIT_FAIL) );
      do {
	n = ntasks - 1;
	CHECK( pip_dequeue_and_resume_N(&expp->queue,task,&n), RV,
	       return(EXIT_FAIL) );
      } while( n < 0 );
      do { 
	pip_yield(PIP_YIELD_USER);
      } while( expp->done < 1 );
      do { 
	pip_yield(PIP_YIELD_USER);
      } while( expp->done < 2 );
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
