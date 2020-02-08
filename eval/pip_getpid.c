/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#define NOFSYNC

#include <eval.h>
#include <test.h>

#define NSAMPLES	(10)
#define WITERS		(1000)
#define NITERS		(10*1000)
#ifdef AH
#define WITERS		(10)
#define NITERS		(100)
#endif

typedef struct exp {
  pip_barrier_t		barrier0;
  pip_barrier_t		barrier1;
  pip_task_queue_t	queue;
  volatile int		done;
} exp_t;

#ifdef NTASKS
#undef NTASKS
#endif

#ifdef AH
#define NTASKS		(5)
static int opts[NTASKS] = { PIP_SYNC_BUSYWAIT,
			    PIP_SYNC_YIELD,
			    PIP_SYNC_BLOCKING,
			    PIP_SYNC_AUTO,
			    PIP_SYNC_AUTO };

static char *optstr[NTASKS] = { "PIP_SYNC_BUSYWAIT",
				"PIP_SYNC_YIELD",
				"PIP_SYNC_BLOCKING",
				"PIP_SYNC_AUTO",
				"PIP_SYNC_AUTO" };
#endif

#define NTASKS		(4)

static int opts[NTASKS] = { PIP_SYNC_AUTO,
			    PIP_SYNC_BUSYWAIT,
			    PIP_SYNC_BLOCKING,
			    PIP_SYNC_AUTO };

static char *optstr[NTASKS] = { "PIP_SYNC_AUTO",
				"PIP_SYNC_BUSYWAIT",
				"PIP_SYNC_BLOCKING",
				"PIP_SYNC_AUTO" };

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	status, i, extval = 0;
  double nd = (double) niters;

  ntasks = NTASKS;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&expp->barrier0, ntasks), RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_init(&expp->barrier1, 2     ), RV, return(EXIT_FAIL) );
    CHECK( pip_task_queue_init(&expp->queue, NULL  ),  RV, return(EXIT_FAIL) );
    expp->done = 0;

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_blt_spawn( &prog, i, 0, &pipid, NULL, NULL, NULL ),
	     RV,
	     abend(EXIT_UNTESTED) );
    }
    for( i=0; i<ntasks; i++ ) {
      CHECK( pip_wait(i,&status), RV, return(EXIT_FAIL) );
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
      double 	t, min0, min1, t0[NSAMPLES], t1[NSAMPLES];
      uint64_t	c, c0[NSAMPLES], c1[NSAMPLES];
      pid_t	pid0, pid1;
      int 	j, idx0, idx1;

      CHECK( pip_barrier_wait(&expp->barrier0),               RV, return(EXIT_FAIL) );
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      CHECK( pip_set_syncflag( opts[pipid] ),                 RV, return(EXIT_FAIL) );

      for( i=0; i<niters*1000; i++ ) {
	pip_gettime();
	get_cycle_counter();
	pid0 = getpid();
      }

      for( j=0; j<10; j++ ) {
	/* getpid() */
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
	c0[j] = ( get_cycle_counter() - c ) / niters;
	t0[j] = ( pip_gettime() - t ) / nd;

	/* pip_couple(); getpid(); pip_decouple() */
	for( i=0; i<witers; i++ ) {
	  c1[j] = 0;
	  t1[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  CHECK( pip_couple(),       RV, return(EXIT_FAIL) );
	  pid1 = getpid();
	  CHECK( pip_decouple(NULL), RV, return(EXIT_FAIL) );
	}

	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  CHECK( pip_couple(),       RV, return(EXIT_FAIL) );
	  pid1 = getpid();
	  CHECK( pip_decouple(NULL), RV, return(EXIT_FAIL) );
	}
	c1[j] = ( get_cycle_counter() - c ) / niters;;
	t1[j] = ( pip_gettime() - t ) / nd;
      }

      printf( "SYNC_OPT: %s\n", optstr[pipid] );
      min0 = t0[0];
      min1 = t1[0];
      idx0 = idx1 = 0;
      for( j=0; j<NSAMPLES; j++ ) {
	printf( "[%d] getpid()     : %g  (%lu)\n", j, t0[j], c0[j] );
	printf( "[%d] pip:getpid() : %g  (%lu)\n", j, t1[j], c1[j] );
	if( min0 > t0[j] ) {
	  min0 = t0[j];
	  idx0 = j;
	}
	if( min1 > t1[j] ) {
	  min1 = t1[j];
	  idx1 = j;
	}
      }
      printf( "[[%d]] getpid()     : %g  (%lu)\n", idx0, t0[idx0], c0[idx0] );
      printf( "[[%d]] pip:getpid() : %g  (%lu)\n", idx1, t1[idx1], c1[idx1] );
      printf( "--dummy %d %d\n", pid0, pid1 );
      fflush( NULL );
      expp->done ++;
      pip_memory_barrier();
      CHECK( pip_barrier_wait(&expp->barrier1), RV, return(EXIT_FAIL) );

    } else {
      pip_task_t *task;
      int ql;

      CHECK( pip_barrier_wait(&expp->barrier0),               RV, return(EXIT_FAIL) );
      CHECK( pip_get_task_from_pipid(PIP_PIPID_MYSELF,&task), RV, return(EXIT_FAIL) );
      while( 1 ) {
	CHECK( pip_task_queue_count(&expp->queue,&ql),        RV, return(EXIT_FAIL) );
	if( ql == ntasks - 1 ) break;
	pip_yield( PIP_YIELD_DEFAULT );
      }
      for( i=0; i<ntasks-1; i++ ) {
	CHECK( pip_dequeue_and_resume(&expp->queue,task),     RV, return(EXIT_FAIL) );
	do {
	  pip_yield( PIP_YIELD_USER );
	} while( expp->done == i );
	CHECK( pip_barrier_wait(&expp->barrier1), RV, return(EXIT_FAIL) );
      }
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
