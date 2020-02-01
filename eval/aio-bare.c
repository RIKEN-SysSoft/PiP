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

#define BUFSZ	(4*4096)
char buffer[BUFSZ];

#define NSAMPLES	(10)
#define WITERS		(1000)
#define NITERS		(10*1000)

struct aiocb cb;

char *fname = "/tmp/tmp.del";

void aio( void ) {
  int fd;

  errno = 0;
  memset( &cb, 0, sizeof(cb) );
  fd = creat( fname, S_IRWXU );
  if( errno ) {
    printf( "%d: Err:%d\n", __LINE__, errno );
    exit( 1 );
  }
  cb.aio_fildes  = fd;
  cb.aio_offset  = 0;
  cb.aio_buf     = buffer;
  cb.aio_nbytes  = BUFSZ;
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
  unlink(fname);
}

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

int main() {
  double 	t, ts[NSAMPLES];
  int		niters = NITERS, witers=WITERS;
  int		i, j;
  
  unlink(fname);
  for( j=0; j<NSAMPLES; j++ ) {  
    for( i=0; i<witers; i++ ) {
      ts[j] = 0.0;
      pip_gettime();
      aio();
    }
    t = pip_gettime();
    for( i=0; i<niters; i++ ) aio();
    ts[j] = pip_gettime() - t;
  }

  double nd = (double) niters;
  for( j=0; j<NSAMPLES; j++ ) {  
    printf( "[%d] aio_write : %g\n", j, ts[j] / nd );
  }
  return 0;
}
