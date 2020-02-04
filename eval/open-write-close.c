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
#define NITERS		(1000)

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

#define NTASKS		(sizeof(opts)/sizeof(int))

char *fname = "/tmp/tmp.del";

#define BUFSZ	(128*1024)
char buffer[BUFSZ];

int open_write_close( size_t sz ) {
  int fd;

  CHECK( (fd=open( fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU )), RV<0, exit(EXIT_FAIL) );
  CHECK( write(fd,buffer,sz),          RV<0, exit(EXIT_FAIL) );
#ifndef NOFSYNC
  CHECK( fsync(fd),                    RV,   exit(EXIT_FAIL) );
#endif
  CHECK( close(fd),                    RV,   exit(EXIT_FAIL) );
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
    double 	t, tt, t0[NSAMPLES], tt0[NSAMPLES];
    char	*fname = "/tmp/pip_couple.del";
    double	min0 = t0[0];
    int 	idx0 = 0;
    size_t	sz;

    CHECK( pip_barrier_wait(&expp->barrier),                RV, return(EXIT_FAIL) );

    if( pipid < ntasks - 2 ) {
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      CHECK( pip_set_syncflag( opts[pipid] ),                 RV, return(EXIT_FAIL) );

      for( sz=4096; sz<BUFSZ; sz*=2 ) {
	printf( "SYNC_OPT: %s  SZ:%lu\n", optstr[pipid], sz );
	fflush( NULL );
	sync(); sync(); sync();
	sleep( 3 );

	for( j=0; j<NSAMPLES; j++ ) {
	  for( i=0; i<witers; i++ ) {
	    t0[j]  = 0.0;
	    tt0[j] = 0.0;
	    pip_gettime();
	    memset( buffer, 123, sz );
	    CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	    open_write_close( sz );
	    CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	  }
	  t = pip_gettime();
	  for( i=0; i<niters; i++ ) {
	    memset( buffer, 123, sz );
	    CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	    tt = pip_gettime();
	    open_write_close( sz );
	    tt0[j] += pip_gettime() - tt;
	    CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	  }
	  t0[j] = ( pip_gettime() - t ) / nd;
	  tt0[j] /= nd;
	}
	min0 = t0[0];
	idx0 = 0;
	for( j=0; j<NSAMPLES; j++ ) {
	  printf( "[%d] pip:open-close : %g  %g\n", j, t0[j], tt0[j] );
	  if( min0 > t0[j] ) {
	    min0 = t0[j];
	    idx0 = j;
	  }
	}
	printf( "[[%d]] %lu pip:open-close : %.3g  %.3g\n", idx0, sz, t0[idx0], tt0[idx0] );
	fflush( NULL );
      }
      expp->done ++;

    } else if( pipid == ntasks - 2 ) {
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );

      for( sz=4096; sz<BUFSZ; sz*=2 ) {
	printf( "Linux  SZ:%lu\n", sz );
	fflush( NULL );
	sync(); sync(); sync();
	sleep( 3 );
	for( j=0; j<NSAMPLES; j++ ) {
	  unlink(fname);
	  sync(); sync(); sync();
	  sleep( 1 );
	  /* creat() close(); unlink() */
	  for( i=0; i<witers; i++ ) {
	    t0[j] = 0.0;
	    pip_gettime();
	    memset( buffer, 123, sz );
	    open_write_close( sz );
	  }
	  t = pip_gettime();
	  for( i=0; i<niters; i++ ) {
	    memset( buffer, 123, sz );
	    open_write_close( sz );
	  }
	  t0[j] = ( pip_gettime() - t ) / nd;
	}
	min0 = t0[0];
	idx0 = 0;
	for( j=0; j<NSAMPLES; j++ ) {
	  printf( "[%d] open-close     : %g\n", j, t0[j] );
	  if( min0 > t0[j] ) {
	    min0 = t0[j];
	    idx0 = j;
	  }
	}
	printf( "[[%d]] %lu open-close     : %.3g\n", idx0, sz, t0[idx0] );
	fflush( NULL );
      }
      expp->done ++;

    } else {
      pip_task_t *task;
      int ql;

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
