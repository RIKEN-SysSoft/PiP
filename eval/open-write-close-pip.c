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

#include <open-write-close.h>

#ifdef NTASKS
#undef NTASKS
#endif

static int opts[] = { PIP_SYNC_BUSYWAIT,
		      PIP_SYNC_BLOCKING,
		      0 };

static char *optstr[] = { "PIP_SYNC_BUSYWAIT",
			  "PIP_SYNC_BLOCKING",
			  "" };

#define NTASKS		(sizeof(opts)/sizeof(int))

typedef struct exp {
  pip_barrier_t		barrier0;
  pip_barrier_t		barrier1;
  pip_task_queue_t	queue;
  volatile double 	tp0[NSAMPLES], to0[NSAMPLES], tc0[NSAMPLES];
} exp_t;

char *fname = "/tmpfs/tmp.del";

char buffer[BUFSZ];

int open_write_close( size_t sz ) {
  int fd;

  fd = open( fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
  CHECK( write(fd,buffer,sz),          RV<0, return(EXIT_FAIL) );
#ifndef NOFSYNC
  CHECK( fsync(fd),                    RV,   return(EXIT_FAIL) );
#endif
  CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
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

  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&expp->barrier0,ntasks), RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_init(&expp->barrier1,2     ), RV, return(EXIT_FAIL) );
    CHECK( pip_task_queue_init(&expp->queue ,NULL),  RV, return(EXIT_FAIL) );

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
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
    double	min0;
    int 	idx0 = 0;
    size_t	sz;

    CHECK( pip_barrier_wait(&expp->barrier0),                RV, return(EXIT_FAIL) );

    if( pipid < ntasks - 1 ) {
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      CHECK( pip_set_syncflag( opts[pipid] ),                 RV, return(EXIT_FAIL) );

      for( sz=IOSZ0; sz<BUFSZ; sz*=2 ) {
	printf( "SYNC_OPT: %s  SZ:%lu\n", optstr[pipid], sz );
	fflush( NULL );

	for( j=0; j<NSAMPLES; j++ ) {
	  for( i=0; i<witers; i++ ) {
	    expp->tp0[j] = 0.0;
	    pip_gettime();
	    memset( buffer, 123, sz );
	    CHECK( pip_couple(),       RV, return(EXIT_FAIL) );
	    open_write_close( sz );
	    CHECK( pip_decouple(NULL), RV, return(EXIT_FAIL) );
	  }
	  CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	  for( i=0; i<niters; i++ ) {
	    expp->tp0[j] -= pip_gettime();
	    memset( buffer, 123, sz );
	    CHECK( pip_couple(),       RV, return(EXIT_FAIL) );
	    open_write_close( sz );
	    CHECK( pip_decouple(NULL), RV, return(EXIT_FAIL) );
	    expp->tp0[j] += pip_gettime();
	  }
	  expp->tp0[j] /= nd;
	  CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	  CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	  for( i=0; i<niters; i++ ) {
	    memset( buffer, 123, sz );
	    double t = pip_gettime();
	    expp->to0[j] -= t;
	    CHECK( pip_couple(),       RV, return(EXIT_FAIL) );
	    open_write_close( sz );
	    CHECK( pip_decouple(NULL), RV, return(EXIT_FAIL) );
	    expp->to0[j] += pip_gettime();
	  }
	  expp->to0[j] /= nd;
	  while( pip_yield( PIP_YIELD_DEFAULT ) == 0 );
	}
	min0 = expp->tp0[0];
	idx0 = 0;
	for( j=0; j<NSAMPLES; j++ ) {
	  printf( "[%d] pip:open-close : %g\n", j, expp->tp0[j] );
	  if( min0 > expp->tp0[j] ) {
	    min0 = expp->tp0[j];
	    idx0 = j;
	  }
	}
	double ov;
	printf( "tp:%g  tc:%g  to:%g\n", expp->tp0[idx0], expp->tc0[idx0], expp->to0[idx0] );
	ov = 100. * max( 0., min( 1., ( expp->tp0[idx0] + expp->tc0[idx0] - expp->to0[idx0] ) /
				  min( expp->tp0[idx0], expp->tc0[idx0] ) ) );
	printf( "[[%d]] %lu pip:open-close : %.3g  %.3g\n", idx0, sz, expp->tp0[idx0], ov );
	fflush( NULL );
      }

    } else {
      pip_task_t *task;
      int ql, k;

      CHECK( pip_get_task_from_pipid(PIP_PIPID_MYSELF,&task), RV, return(EXIT_FAIL) );
      while( 1 ) {
	CHECK( pip_task_queue_count(&expp->queue,&ql),        RV, return(EXIT_FAIL) );
	if( ql == ntasks - 1 ) break;
	pip_yield( PIP_YIELD_DEFAULT );
      }
      for( k=0; k<(ntasks-1); k++ ) {
	printf( ">> k:%d / %d\n", k, ntasks );
 	CHECK( pip_dequeue_and_resume(&expp->queue,task),     RV, return(EXIT_FAIL) );
	for( sz=IOSZ0; sz<BUFSZ; sz*=2 ) {
	  for( j=0; j<NSAMPLES; j++ ) {
	    for( i=0; i<witers; i++ ) {
	      while( pip_yield( PIP_YIELD_DEFAULT ) == 0 );
	      expp->tc0[j] = 0.0;
	    }
	    CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	    for( i=0; i<niters; i++ ) {
	      while( pip_yield( PIP_YIELD_DEFAULT ) == 0 );
	    }
	    CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	    IMB_cpu_exploit( (float) expp->tp0[j], 1 );
	    for( i=0; i<witers; i++ ) {
	      IMB_cpu_exploit( (float) expp->tp0[j], 0 );
	    }
	    CHECK( pip_barrier_wait(&expp->barrier1),                RV, return(EXIT_FAIL) );
	    for( i=0; i<niters; i++ ) {
	      while( pip_yield( PIP_YIELD_DEFAULT ) == 0 );
	      expp->tc0[j] -= pip_gettime();
	      IMB_cpu_exploit( (float) expp->tp0[j], 0 );
	      expp->tc0[j] += pip_gettime();
	    }
	    expp->tc0[j] /= nd;
	    while( pip_yield( PIP_YIELD_DEFAULT ) == 0 );
	  }
	}
      }
    }
  }
  printf( "[%d] done\n", pipid );
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
