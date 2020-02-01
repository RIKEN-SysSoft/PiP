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

void *foo( void *dummy ) {
  int i, j;
  
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
}

int main() {
  double 	t, ts0[NSAMPLES], ts1[NSAMPLES];
  int		i, j;
  pthread_t	th[2];
  pthread_attr_t attr[2];
  cpu_set_t	cpuset[2];

  pthread_attr_init( &(attr[0]) );
  CPU_ZERO( &(cpuset[0]) );
  CPU_SET( 1, &(cpuset[0]) );
  pthread_attr_setaffinity_np( &(attr[0]), sizeof(cpuset), &(cpuset[0]) );

  pthread_attr_init( &(attr[1]) );
  CPU_ZERO( &(cpuset[1]) );
  CPU_SET( 2, &(cpuset[1]) );
  pthread_attr_setaffinity_np( &(attr[1]), sizeof(cpuset), &(cpuset[1]) );

  pthread_barrier_init( &barr, NULL, 3 );
  pthread_create( &(th[0]), &(attr[0]), foo, NULL );
  pthread_create( &(th[1]), &(attr[1]), foo, NULL );
  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {  
    for( i=0; i<witers; i++ ) {
      pip_gettime();
    }
    pthread_barrier_wait( &barr );
    t = pip_gettime();
    pthread_barrier_wait( &barr );
    ts0[j] = pip_gettime() - t;
  }
  pthread_join( th[0], NULL );
  pthread_join( th[1], NULL );

  pthread_barrier_init( &barr, NULL, 3 );
  pthread_create( &(th[0]), &(attr[0]), foo, NULL );
  pthread_create( &(th[1]), &(attr[0]), foo, NULL );
  pthread_barrier_wait( &barr );
  for( j=0; j<NSAMPLES; j++ ) {  
    for( i=0; i<witers; i++ ) {
      pip_gettime();
    }
    pthread_barrier_wait( &barr );
    t = pip_gettime();
    pthread_barrier_wait( &barr );
    ts1[j] = pip_gettime() - t;
  }
  pthread_join( th[0], NULL );
  pthread_join( th[1], NULL );

  double nd = (double) niters;
  for( j=0; j<NSAMPLES; j++ ) {  
    printf( "[%d] pthread_sched : %g  %g\n", j, ts0[j] / nd, ts1[j] / nd );
  }
  return 0;
}
