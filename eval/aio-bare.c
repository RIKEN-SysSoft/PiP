/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

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

static void aio_busywait( int sz ) {
  int fd;

  memset( buffer, 123, sz );
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
  while( 1 ) {
    if( aio_return( &cb ) > 0 ) break;
  }
  close(fd);
}

static void aio_block( int sz ) {
  struct aiocb *cba[1];
  int fd;

  memset( buffer, 123, sz );
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
  cba[0] = &cb;
  aio_suspend( (const struct aiocb * const*)cba, 1, NULL );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
  close(fd);
}

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

int main() {
  int		niters = NITERS, witers=WITERS;
  int		i, j, k;
  double 	t, ts0[NSAMPLES], ts1[NSAMPLES];
  double	nd = (double) niters;

  for( k=IOSZ0; k<BUFSZ; k*=2 ) {
    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	ts0[j] = 0.0;
	t = pip_gettime();
	aio_busywait( k );
      }
      for( i=0; i<niters; i++ ) {
	t = pip_gettime();
	aio_busywait( k );
	ts0[j] += pip_gettime() - t;
      }
      ts0[j] /= nd;
    }

    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	ts1[j] = 0.0;
	t = pip_gettime();
	aio_block( k );
      }
      for( i=0; i<niters; i++ ) {
	t = pip_gettime();
	aio_block( k );
	ts1[j] += pip_gettime() - t;
      }
      ts1[j] /= nd;
    }
    double min0 = ts0[0], min1 = ts1[0];
    int    idx0 = 0, idx1 = 0;
    for( j=0; j<NSAMPLES; j++ ) {
      printf( "[%d] aio-return : %g  %g\n", j, ts0[j], ts1[j] );
      if( min0 > ts0[j] ) {
	min0 = ts0[j];
	idx0 = j;
      }
      if( min1 > ts1[j] ) {
	min1 = ts1[j];
	idx1 = j;
      }
    }
    printf( "[[%d]] %d aio-return  : %.3g\n", idx0, k, ts0[idx0] );
    printf( "[[%d]] %d aio-suspend : %.3g\n", idx1, k, ts1[idx1] );
    fflush( NULL );
  }
  return 0;
}
