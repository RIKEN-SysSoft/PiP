/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>

#define NSAMPLES	(10)
#define WITERS		(10*000)
#define NITERS		(1000*1000)

int	niters = NITERS, witers=WITERS;
pthread_barrier_t barr;

double pip_gettime( void ) {
  struct timeval tv;
  gettimeofday( &tv, NULL );
  return ((double)tv.tv_sec + (((double)tv.tv_usec) * 1.0e-6));
}

void *foo1( void *dummy ) {
  cpu_set_t	cpuset;
  int i, j;

  CPU_ZERO( &cpuset );
  CPU_SET( 1, &cpuset );
  pthread_setaffinity_np( pthread_self(), sizeof(cpuset), &cpuset );

  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {
    for ( i=0; i<witers; i++ ) {
      (void) pthread_yield();
    }
    pthread_barrier_wait( &barr );
    for ( i=0; i<niters; i++ ) {
      (void) pthread_yield();
    }
    pthread_barrier_wait( &barr );
  }
  return NULL;
}

void *foo2( void *dummy ) {
  cpu_set_t	cpuset;
  int i, j;

  CPU_ZERO( &cpuset );
  CPU_SET( 2, &cpuset );
  pthread_setaffinity_np( pthread_self(), sizeof(cpuset), &cpuset );

  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {
    for ( i=0; i<witers; i++ ) {
      (void) pthread_yield();
    }
    pthread_barrier_wait( &barr );
    for ( i=0; i<niters; i++ ) {
      (void) pthread_yield();
    }
    pthread_barrier_wait( &barr );
  }
  return NULL;
}

int main() {
  double 	t, ts0[NSAMPLES], ts1[NSAMPLES];
  double	nd = (double) ( niters * 2 );
  int		i, j;
  pthread_t	th[2];

  pthread_barrier_init( &barr, NULL, 3 );
  pthread_create( &(th[0]), NULL, foo1, NULL );
  pthread_create( &(th[1]), NULL, foo1, NULL );
  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {
    for( i=0; i<witers; i++ ) {
      t = pip_gettime();
    }
    pthread_barrier_wait( &barr );
    t = pip_gettime();
    pthread_barrier_wait( &barr );
    ts0[j] = ( pip_gettime() - t ) /nd;
  }
  pthread_join( th[0], NULL );
  pthread_join( th[1], NULL );

  pthread_barrier_init( &barr, NULL, 3 );
  pthread_create( &(th[0]), NULL, foo1, NULL );
  pthread_create( &(th[1]), NULL, foo2, NULL );
  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {
    for( i=0; i<witers; i++ ) {
      t = pip_gettime();
    }
    pthread_barrier_wait( &barr );
    t = pip_gettime();
    pthread_barrier_wait( &barr );
    ts1[j] = ( pip_gettime() - t ) /nd;
  }
  pthread_join( th[0], NULL );
  pthread_join( th[1], NULL );

  double min0 = ts0[0], min1 = ts1[0];
  for( j=0; j<NSAMPLES; j++ ) {
    printf( "[%d] pthread_sched : %g  %g\n", j, ts0[j], ts1[j] );
    min0 = ( min0 > ts0[j] ) ? ts0[j] : min0;
    min1 = ( min1 > ts0[j] ) ? ts0[j] : min1;
  }
  printf( "[%d] MIN : %g  %g\n", j, min0, min1 );
  return 0;
}
