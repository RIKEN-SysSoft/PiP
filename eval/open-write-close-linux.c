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

#define NTASKS		(2)

char *fname = "/tmpfs/tmp.del";

#define BUFSZ	(128*1024)
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
  int	witers = WITERS, niters = NITERS;
  int	i, j;
  double nd = (double) niters;
  double t, t0[NSAMPLES], min0;
  int 	idx0;
  size_t	sz;

  for( sz=4096; sz<BUFSZ; sz*=2 ) {
    printf( "Linux  SZ:%lu\n", sz );
    fflush( NULL );

    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	t0[j] = 0.0;
	t = pip_gettime();
	memset( buffer, 123, sz );
	open_write_close( sz );
      }
      for( i=0; i<niters; i++ ) {
	t = pip_gettime();
	memset( buffer, 123, sz );
	open_write_close( sz );
	t0[j] += pip_gettime() - t;
      }
      t0[j] /= nd;
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
  return 0;
}
