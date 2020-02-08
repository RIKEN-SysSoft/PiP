/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#define _GNU_SOURCE
#include <sched.h>
#include <aio.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <open-write-close.h>

char buffer[BUFSZ];

struct aiocb cb;

char *fname = "/tmpfs/tmp.del";

static int aio_openwrite( int sz ) {
  int		fd;

  errno = 0;
  memset( &cb, 0, sizeof(cb) );
  fd = open( fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
  cb.aio_fildes  = fd;
  cb.aio_offset  = 0;
  cb.aio_buf     = buffer;
  cb.aio_nbytes  = sz;
  cb.aio_sigevent.sigev_notify = SIGEV_NONE;
  aio_write( &cb );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
  return fd;
}

static void aio_busywait( void ) {
  while( 1 ) {
    if( aio_return( &cb ) > 0 ) break;
  }
}

static void aio_block( void ) {
  struct aiocb *cba[1];

  cba[0] = &cb;
  aio_suspend( (const struct aiocb * const*)cba, 1, NULL );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
}

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

int main() {
  int		niters = NITERS, witers=WITERS;
  int		fd, i, j, sz;
  double 	tp0[NSAMPLES], tp1[NSAMPLES];
  double	to0[NSAMPLES], to1[NSAMPLES];
  double	tc0[NSAMPLES], tc1[NSAMPLES];
  double	nd = (double) niters;

#ifdef AH
  cpu_set_t	cpuset;
  CPU_ZERO( &cpuset );
  CPU_SET( 1, &cpuset );
  CPU_SET( 2, &cpuset );
  sched_setaffinity( getpid(), sizeof(cpuset), &cpuset );
#endif

  for( sz=IOSZ0; sz<BUFSZ; sz*=2 ) {
    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	tp0[j] = 0.0;
	to0[j] = 0.0;
	tc0[j] = 0.0;
	pip_gettime();
	memset( buffer, 123, sz );
	fd = aio_openwrite( sz );
	aio_busywait();
	close( fd );
      }
      for( i=0; i<niters; i++ ) {
	memset( buffer, 123, sz );
	tp0[j] -= pip_gettime();
	pip_gettime();
	fd = aio_openwrite( sz );
	aio_busywait();
	close( fd );
	tp0[j] += pip_gettime();
      }
      tp0[j] /= nd;
      for( i=0; i<witers; i++ ) {
        IMB_cpu_exploit( (float) tp0[j], 1 );
      }
      for( i=0; i<niters; i++ ) {
	memset( buffer, 123, sz );
	to0[j] -= pip_gettime();
	fd = aio_openwrite( sz );
	tc0[j] -= pip_gettime();
        IMB_cpu_exploit( (float) tp0[j], 0 );
	tc0[j] += pip_gettime();
	aio_busywait();
	close( fd );
	to0[j] += pip_gettime();
      }
      to0[j] /= nd;
      tc0[j] /= nd;
    }

    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	tp1[j] = 0.0;
	to1[j] = 0.0;
	tc1[j] = 0.0;
	pip_gettime();
	memset( buffer, 123, sz );
	fd = aio_openwrite( sz );
	aio_block();
	close( fd );
      }
      for( i=0; i<niters; i++ ) {
	memset( buffer, 123, sz );
	tp1[j] -= pip_gettime();
	pip_gettime();
	fd = aio_openwrite( sz );
	aio_block();
	close( fd );
	tp1[j] += pip_gettime();
      }
      tp1[j] /= nd;
      for( i=0; i<witers; i++ ) {
        IMB_cpu_exploit( (float) tp1[j], 1 );
      }
      for( i=0; i<niters; i++ ) {
	memset( buffer, 123, sz );
	to1[j] -= pip_gettime();
	fd = aio_openwrite( sz );
	tc1[j] -= pip_gettime();
        IMB_cpu_exploit( (float) tp1[j], 0 );
	tc1[j] += pip_gettime();
	aio_block();
	close( fd );
	to1[j] += pip_gettime();
      }
      to1[j] /= nd;
      tc1[j] /= nd;
    }
    double min0 = tp0[0], min1 = tp1[0];
    int    idx0 = 0, idx1 = 0;
    for( j=0; j<NSAMPLES; j++ ) {
      printf( "[%d] aio-return : %g  %g\n", j, tp0[j], tp1[j] );
      if( min0 > tp0[j] ) {
	min0 = tp0[j];
	idx0 = j;
      }
      if( min1 > tp1[j] ) {
	min1 = tp1[j];
	idx1 = j;
      }
    }
    double ov;
    ov = 100. * max( 0., min( 1., (tp0[idx0]+tc0[idx0]-to0[idx0]) / min( tp0[idx0], tc0[idx0] ) ) );
    printf( "[[%d]] %d aio-return  : %.3g  %.3g\n", idx0, sz, to0[idx0], ov );
    printf( "tp:%g  tc:%g  to:%g\n", tp0[idx0], tc0[idx0], to0[idx0] );
    ov = 100. * max( 0., min( 1., (tp1[idx1]+tc1[idx1]-to1[idx1]) / min( tp1[idx1], tc1[idx1] ) ) );
    printf( "[[%d]] %d aio-suspend : %.3g  %.3g\n", idx1, sz, to1[idx1], ov );
    printf( "tp:%g  tc:%g  to:%g\n", tp1[idx1], tc1[idx1], to1[idx1] );
    fflush( NULL );
  }
  return 0;
}
