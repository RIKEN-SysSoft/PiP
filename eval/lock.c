/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>
#include <semaphore.h>

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
  double	t0, t1;

  sem_init( &sem[0], 0, 0 );
  sem_init( &sem[1], 0, 0 );

  pthread_mutex_init( &mutex[0], NULL );
  pthread_mutex_init( &mutex[1], NULL );
  pthread_mutex_lock( &mutex[0] );

  pthread_create( &th0, NULL, foo, NULL );
  pthread_create( &th1, NULL, bar, NULL );

  for( j=0; j<10; j++ ) {
    for( i=0; i<witers; i++ ) {
      pip_gettime();
      sem_post( &sem[0] );
      sem_wait( &sem[1] );
    }  
    t0 = - pip_gettime();
    for( i=0; i<niters; i++ ) {
      sem_post( &sem[0] );
      sem_wait( &sem[1] );
    }
    t0 += pip_gettime();
    
    for( i=0; i<witers; i++ ) {
      pip_gettime();
      pthread_mutex_unlock( &mutex[0] );
      pthread_mutex_lock(   &mutex[1] );
    }  
    t1 = - pip_gettime();
    for( i=0; i<niters; i++ ) {
      pthread_mutex_unlock( &mutex[0] );
      pthread_mutex_lock(   &mutex[1] );
    }
    t1 += pip_gettime();

    printf( "sem   : %g\n", t0 / ((double) niters) );
    printf( "mutex : %g\n", t1 / ((double) niters) );
  }
  return 0;
}
