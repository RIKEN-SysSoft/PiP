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

#define BUFSZ	(128*1024)
char buffer[BUFSZ];

#include <open-write-close.h>

struct aiocb cb;

char *fname = "/tmpfs/tmp.del";

void aio( int sz ) {
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

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

int main() {
  int		niters = NITERS, witers=WITERS;
  int		i, j, k;
  double 	t, ts[NSAMPLES];
  double	nd = (double) niters;

  for( k=4096; k<BUFSZ; k*=2 ) {
    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	ts[j] = 0.0;
	t = pip_gettime();
	aio( k );
      }
      for( i=0; i<niters; i++ ) {
	t = pip_gettime();
	aio( k );
	ts[j] += pip_gettime() - t;
      }
      ts[j] /= nd;
    }
    double min = ts[0];
    int    idx = 0;
    for( j=0; j<NSAMPLES; j++ ) {
      printf( "[%d] aio_write : %g\n", j, ts[j] );
      if( min > ts[j] ) {
	min = ts[j];
	idx = j;
      }
    }
    printf( "[[%d]] %d aio_write : %.3g\n", idx, k, ts[idx] );
    fflush( NULL );
  }
  return 0;
}
