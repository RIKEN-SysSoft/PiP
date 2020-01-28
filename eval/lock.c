/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <eval.h>
#include <test.h>
#include <semaphore.h>

#define NSAMPLES	(10)
#define WITERS		(100)
#define NITERS		(10*1000)

sem_t 		sem[2];
pthread_mutex_t mutex[2];

void *foo( void *args ) {
  while( 1 ) {
    sem_wait( &sem[0] );
    sem_post( &sem[1] );
  }
}

void *bar( void *args ) {
  while( 1 ) {
    pthread_mutex_lock(   &mutex[0] );
    pthread_mutex_unlock( &mutex[1] );
  }
}

int main() {
  pthread_t 	th0, th1;
  int		witers = WITERS, niters = NITERS;
  int		i, j;
  double	t, t0[NSAMPLES], t1[NSAMPLES];
  uint64_t	c, c0[NSAMPLES], c1[NSAMPLES];

  sem_init( &sem[0], 0, 0 );
  sem_init( &sem[1], 0, 0 );

  pthread_mutex_init( &mutex[0], NULL );
  pthread_mutex_init( &mutex[1], NULL );
  pthread_mutex_lock( &mutex[0] );

  pthread_create( &th0, NULL, foo, NULL );
  pthread_create( &th1, NULL, bar, NULL );

  for( j=0; j<NSAMPLES; j++ ) {
    for( i=0; i<witers; i++ ) {
      c0[j] = 0;
      t0[j] = 0.0;
      pip_gettime();
      get_cycle_counter();
      sem_post( &sem[0] );
      sem_wait( &sem[1] );
    }
    t = pip_gettime();
    c = get_cycle_counter();
    for( i=0; i<niters; i++ ) {
      sem_post( &sem[0] );
      sem_wait( &sem[1] );
    }
    c0[j] = get_cycle_counter() - c;
    t0[j] = pip_gettime() - t;

    for( i=0; i<witers; i++ ) {
      c1[j] = 0;
      t1[j] = 0.0;
      pip_gettime();
      get_cycle_counter();
      pthread_mutex_unlock( &mutex[0] );
      pthread_mutex_lock(   &mutex[1] );
    }
    t = pip_gettime();
    c = get_cycle_counter();
    for( i=0; i<niters; i++ ) {
      pthread_mutex_unlock( &mutex[0] );
      pthread_mutex_lock(   &mutex[1] );
    }
    c1[j] = get_cycle_counter() - c;
    t1[j] = pip_gettime() - t;
  }
  for( j=0; j<NSAMPLES; j++ ) {
    printf( "sem   : %g  (%lu)\n", t0[j] / ((double) niters), c0[j] / niters );
    printf( "mutex : %g  (%lu)\n", t1[j] / ((double) niters), c1[j] / niters );
  }
  return 0;
}
