/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <eval.h>
#include <test.h>

#define NSAMPLES	(10)
#define WITERS		(1000)
#define NITERS		(10*1000)

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(5)

typedef struct exp {
  pip_task_queue_t	queue;
  volatile int		done;
} exp_t;

int opts[NTASKS] = { PIP_SYNC_BUSYWAIT,
		     PIP_SYNC_YIELD,
		     PIP_SYNC_BLOCKING,
		     PIP_SYNC_AUTO,
		     PIP_SYNC_AUTO };

char *optstr[NTASKS] = { "PIP_SYNC_BUSYWAIT",
			 "PIP_SYNC_YIELD",
			 "PIP_SYNC_BLOCKING",
			 "PIP_SYNC_AUTO",
			 "PIP_SYNC_AUTO" };

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	status, i, n, extval = 0;

  ntasks = NTASKS;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_task_queue_init(&expp->queue ,NULL), RV, return(EXIT_FAIL) );
    expp->done = 0;

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, i, opts[i], &pipid, NULL, NULL, NULL ),
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
    if( pipid < ntasks - 1 ) {
      double 	t, t0[NSAMPLES], t1[NSAMPLES];
      uint64_t	c, c0[NSAMPLES], c1[NSAMPLES];
      pid_t	pid0, pid1;
      int 	j;

      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      n = expp->done;

      for( j=0; j<10; j++ ) {
	for( i=0; i<witers; i++ ) {
	  c0[j] = 0;
	  t0[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  pid0 = getpid();
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  pid0 = getpid();
	}
	c0[j] = get_cycle_counter() - c;
	t0[j] = pip_gettime() - t;

	for( i=0; i<witers; i++ ) {
	  c1[j] = 0;
	  t1[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	  pid1 = getpid();
	  CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
	}

	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  CHECK( pip_couple(),              RV, return(EXIT_FAIL) );
	  pid1 = getpid();
	  CHECK( pip_decouple(NULL),        RV, return(EXIT_FAIL) );
	}
	c1[j] = get_cycle_counter() - c;
	t1[j]= pip_gettime() - t;
      }
      for( j=0; j<NSAMPLES; j++ ) {
	printf( "getpid()  : %g  (%lu)  %s\n", t0[j] / ((double) niters), c0[j] / niters, optstr[n] );
	printf( "couple    : %g  (%lu)  %s\n", t1[j] / ((double) niters), c1[j] / niters, optstr[n] );
      }
      printf( "--dummy %d %d %d\n", n, pid0, pid1 );
      expp->done ++;

    } else {
      pip_task_t *task;
      CHECK( pip_get_task_from_pipid(PIP_PIPID_MYSELF,&task), RV,
	     return(EXIT_FAIL) );
      n = expp->done;
      for( i=0; i<ntasks-1; i++ ) {
	CHECK( pip_dequeue_and_resume(&expp->queue,task), RV,
	       return(EXIT_FAIL) );
	do {
	  pip_yield( PIP_YIELD_USER );
	} while( expp->done == i );
      }
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
