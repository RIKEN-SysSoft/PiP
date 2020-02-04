/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <eval.h>
#include <test.h>

#define NSAMPLES	(10)
#define WITERS		(100)
#define NITERS		(10*1000)

#ifdef NTASKS
#undef NTASKS
#endif

#define NTASKS		(2)

#define BUFSZ	(1024*1024)
char buffer[BUFSZ];

static int feed_pipe( int fd, size_t sz ) {
  return write( fd, buffer, sz );
}

static void drain_pipe( int fd ) {
  bind_core( 3 );
  while( read( fd, buffer, BUFSZ ) > 0 );
}

int main( int argc, char **argv ) {
  int		witers = WITERS, niters = NITERS;
  int		i, j;
  int		pp[2];
  double 	t, t0[NSAMPLES];
  double 	nd = (double) niters;
  double	min0;
  int 		idx0 = 0;
  size_t	sz;
  pid_t		pid;

  CHECK( pipe(pp),         RV,   return(EXIT_FAIL) );
  CHECK( ( pid = fork() ), RV<0, return(EXIT_FAIL) );
  if( pid == 0 ) {
    close( pp[1] );
    drain_pipe( pp[0] );
    exit( 0 );
  }
  close( pp[0] );

  for( sz=4096; sz<BUFSZ; sz*=2 ) {
    printf( "LINUX  SZ:%lu\n", sz );
    fflush( NULL );

    for( j=0; j<NSAMPLES; j++ ) {
      for( i=0; i<witers; i++ ) {
	t0[j]  = 0.0;
	pip_gettime();
	memset( buffer, 123, sz );
	feed_pipe( pp[1], sz );
      }
      t = pip_gettime();
      for( i=0; i<niters; i++ ) {
	memset( buffer, 123, sz );
	feed_pipe( pp[1], sz );
      }
      t0[j] = ( pip_gettime() - t ) / nd;
    }
    min0 = t0[0];
    idx0 = 0;
    for( j=0; j<NSAMPLES; j++ ) {
      printf( "[%d] linux:pipe : %g\n", j, t0[j] );
      if( min0 > t0[j] ) {
	min0 = t0[j];
	idx0 = j;
      }
    }
    printf( "[[%d]] %lu linux:pipe : %.3g\n", idx0, sz, t0[idx0] );
    fflush( NULL );
  }

  close( pp[1] );
  wait( NULL );
  return 0;
}
