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

#define BUFSZ	(128*1024)
char buffer[BUFSZ];

static char	buffer[BUFSZ];

void open_close( void ) {
  int fd;

  CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
  CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
  CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
}

void open_write_close( size_t sz ) {
  int fd;

  CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
  CHECK( write(fd,buffer,sz),          RV<0, return(EXIT_FAIL) );
#ifndef NOFSYNC
  CHECK( fsync(fd),                    RV,   return(EXIT_FAIL) );
#endif
  CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
  CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog;
  exp_t	exprt, *expp;
  int 	ntasks, pipid;
  int	witers = WITERS, niters = NITERS;
  int	status, i, sz, extval = 0;

  ntasks = NTASKS;

  pip_spawn_from_main( &prog, argv[0], argv, NULL );
  expp = &exprt;

  CHECK( pip_init(&pipid,&ntasks,(void*)&expp,PIP_MODE_PROCESS), RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&expp->barrier ,ntasks), RV, return(EXIT_FAIL) );
    CHECK( pip_task_queue_init(&expp->queue ,NULL),  RV, return(EXIT_FAIL) );
    expp->done = 0;

    memset( buffer, 0, sizeof(buffer) );
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
      double 	t,  t2[NSAMPLES], t3[NSAMPLES];
      double	tt, t4[NSAMPLES], tt3[NSAMPLES], t5[NSAMPLES], tt5[NSAMPLES];
      uint64_t	c,  c2[NSAMPLES], c3[NSAMPLES], c4[NSAMPLES], c5[NSAMPLES];
      pid_t	pid0, pid1;
      char	*fname = "/tmp/pip_couple.del";
      int 	fd, j;
      size_t	sz;

      CHECK( pip_barrier_wait(&expp->barrier),                RV, return(EXIT_FAIL) );
      CHECK( pip_suspend_and_enqueue(&expp->queue,NULL,NULL), RV, return(EXIT_FAIL) );
      CHECK( pip_set_syncflag( opts[pipid] ),                 RV, return(EXIT_FAIL) );


      for( j=0; j<NSAMPLES; j++ ) {
	unlink(fname);
	sync(); sync(); sync();
	sleep( 1 );
	/* creat() close(); unlink() */
	for( i=0; i<witers; i++ ) {
	  c2[j] = 0;
	  t2[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  open_close();
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  open_close();
	}
	c2[j] = get_cycle_counter() - c;
	t2[j] = pip_gettime() - t;

	/* pip_couple(); creat() close(); unlink(); pip_decouple */
	for( i=0; i<witers; i++ ) {
	  c3[j] = 0;
	  t3[j] = 0.0;
	  tt3[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	  open_close();
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	  tt = pip_gettime();
	  open_close();
	  tt3[j] += pip_gettime() - tt;
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	c3[j] = get_cycle_counter() - c;
	t3[j] = pip_gettime() - t;

	/* creat(); write(); fsync(), close(); unlink(); */
	for( i=0; i<witers; i++ ) {
	  c4[j] = 0;
	  t4[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  open_write_close( sz );
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  open_write_close( sz );
	  CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
	  CHECK( write(fd,buffer,BUFSZ),       RV<0, return(EXIT_FAIL) );
#ifndef NOFSYNC
	  CHECK( fsync(fd),                    RV,   return(EXIT_FAIL) );
#endif
	  CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
	  CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
	}
	c4[j] = get_cycle_counter() - c;
	t4[j] = pip_gettime() - t;

	/* pip_couple(); creat(); write(); close(); unlink(); pip_decouple() */
	for( i=0; i<witers; i++ ) {
	  c5[j] = 0;
	  t5[j] = 0.0;
	  tt5[j] = 0.0;
	  pip_gettime();
	  get_cycle_counter();
	  CHECK( pip_couple(),                   RV,   return(EXIT_FAIL) );
	  {
	    CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
	    CHECK( write(fd,buffer,BUFSZ),       RV<0, return(EXIT_FAIL) );
#ifndef NOFSYNC
	    CHECK( fsync(fd),                    RV,   return(EXIT_FAIL) );
#endif
	    CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
	    CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
	  }
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	t = pip_gettime();
	c = get_cycle_counter();
	for( i=0; i<niters; i++ ) {
	  CHECK( pip_couple(),                     RV, return(EXIT_FAIL) );
	  {
	    tt = pip_gettime();
	    CHECK( (fd=creat( fname, S_IRWXU )), RV<0, return(EXIT_FAIL) );
	    CHECK( write(fd,buffer,BUFSZ),       RV<0, return(EXIT_FAIL) );
#ifndef NOFSYNC
	    CHECK( fsync(fd),                    RV,   return(EXIT_FAIL) );
#endif
	    CHECK( close(fd),                    RV,   return(EXIT_FAIL) );
	    CHECK( unlink(fname),                RV,   return(EXIT_FAIL) );
	    tt5[j] += pip_gettime() - tt;
	  }
	  CHECK( pip_decouple(NULL),             RV,   return(EXIT_FAIL) );
	}
	c5[j] = get_cycle_counter() - c;
	t5[j] = pip_gettime() - t;

      }
      printf( "SYNC_OPT: %s\n", optstr[pipid] );
      for( j=0; j<NSAMPLES; j++ ) {
	double nd = (double) niters;
	printf( "[%d] getpid()     : %g  (%lu)\n",
		j, t0[j] / nd, c0[j] / niters );
	printf( "[%d] pip:getpid() : %g  (%lu)\n",
		j, t1[j] / nd, c1[j] / niters );
	printf( "[%d] open         : %g  (%lu)\n",
		j, t2[j] / nd, c2[j] / niters );
	printf( "[%d] pip:open     : %g  %g  (%lu)",
		j, t3[j] / nd, tt3[j] / nd, c3[j] / niters );
	printf( "\n" );
	printf( "[%d] fileio       : %g  (%lu)\n",
		j, t4[j] / nd, c4[j] / niters );
	printf( "[%d] pip:fileio   : %g  %g  (%lu)",
		j, t5[j] / nd, tt5[j] / nd, c5[j] / niters );
	printf( "\n" );
      }
      printf( "--dummy %d %d\n", pid0, pid1 );
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
