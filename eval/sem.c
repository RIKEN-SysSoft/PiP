/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>
#include <semaphore.h>

#define WITERS		(10)
#define NITERS		(1000)

sem_t sem[2];

void *foo( void *args ) {
  while( 1 ) {
    sem_wait( &sem[0] );
    sem_post( &sem[1] );
  }
}

int main() {
  pthread_t 	th;
  int		witers = WITERS, niters = NITERS;
  int		i, j;
  double	t0;

  sem_init( &sem[0], 0, 0 );
  sem_init( &sem[1], 0, 0 );
  pthread_create( &th, NULL, foo, NULL );

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

    printf( "sem : %g\n", t0 / ((double) niters) );
  }
  return 0;
}
