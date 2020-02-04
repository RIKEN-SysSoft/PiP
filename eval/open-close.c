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
#define WITERS		(100)
#define NITERS		(10*1000)
#ifdef AH
#define NSAMPLES	(1)
#define WITERS		(10)
#define NITERS		(10)
#endif

typedef struct exp {
  pip_barrier_t		barrier;
  pip_task_queue_t	queue;
  volatile int		done;
} exp_t;

#ifdef NTASKS
#undef NTASKS
#endif

static int opts[] = { PIP_SYNC_BUSYWAIT,
		      PIP_SYNC_YIELD,
		      PIP_SYNC_BLOCKING,
		      PIP_SYNC_AUTO,
		      PIP_SYNC_AUTO };

static char *optstr[] = { "PIP_SYNC_BUSYWAIT",
			  "PIP_SYNC_YIELD",
			  "PIP_SYNC_BLOCKING",
			  "PIP_SYNC_AUTO",
			  "PIP_SYNC_AUTO" };

#define NSYNCS		((sizeof(opts)/sizeof(int))-1)
#define NTASKS		(NSYNCS+1)

char *fname = "/tmp/tmp.del";

int open_close( void ) {
  int fd;

  CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
  CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
  CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
  return 0;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	i, j, status, extval = 0;
  double nd = (double) niters;

  ntasks = NTASKS;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&expp->barrier ,ntasks), RV, return(EXIT_FAIL) );
    CHECK( pip_task_queue_init(&expp->queue ,NULL),  RV, return(EXIT_FAIL) );
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
      double 	t, tt, t0[NSAMPLES], t1[NSAMPLES], tt1[NSAMPLES];
      char	*fname = "/tmp/pip_couple.del";

      CHECK( pip_barrier_wait(&expp->barrier),                RV, return(EXIT_FAIL) );
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      CHECK( pip_set_syncflag( opts[pipid] ),                 RV, return(EXIT_FAIL) );

      printf( "SYNC_OPT: %s\n", optstr[pipid] );
      fflush( NULL );
      for( j=0; j<NSAMPLES; j++ ) {
	unlink(fname);
	sync(); sync(); sync();
	sleep( 1 );
	/* creat() close(); unlink() */
	for( i=0; i<witers; i++ ) {
	  t0[j] = 0.0;
	  pip_gettime();
	  open_close();
	}
	t = pip_gettime();
	for( i=0; i<niters; i++ ) {
	  open_close();
	}
	t0[j] = ( pip_gettime() - t ) / nd;

	/* pip_couple(); creat() close(); unlink(); pip_decouple */
	for( i=0; i<witers; i++ ) {
	  t1[j]  = 0.0;
	  tt1[j] = 0.0;
	  pip_gettime();
	  CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	  open_close();
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	t = pip_gettime();
	for( i=0; i<niters; i++ ) {
	  CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	  tt = pip_gettime();
	  open_close();
	  tt1[j] += pip_gettime() - tt;
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	t1[j] = ( pip_gettime() - t ) / nd;
	tt1[j] /= nd;
      }
      double min0 = t0[0];
      double min1 = t1[0];
      int idx0 = 0;
      int idx1 = 0;
      for( j=0; j<NSAMPLES; j++ ) {
	printf( "[%d] open-close     : %g\n", j, t0[j] );
	printf( "[%d] pip:open-close : %g  %g\n", j, t1[j], tt1[j] );
	if( min0 > t0[j] ) {
	  min0 = t0[j];
	  idx0 = j;
	}
	if( min1 > t1[j] ) {
	  min1 = t1[j];
	  idx1 = j;
	}
      }
      printf( "[[%d]] open-close     : %.3g\n", idx0, t0[idx0] );
      printf( "[[%d]] pip:open-close : %.3g  %.3g\n", idx1, t1[idx1], tt1[idx1] );
      fflush( NULL );
      expp->done ++;

    } else {
      pip_task_t *task;
      int ql;

      CHECK( pip_barrier_wait(&expp->barrier),                RV, return(EXIT_FAIL) );
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
      }
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
