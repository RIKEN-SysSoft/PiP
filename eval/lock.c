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
#define WITERS		(1000)
#define NITERS		(1000*1000)

sem_t 		sem[2];
pthread_mutex_t mutex[2];

void *foo( void *args ) {
  {
    pid_t tid = pip_gettid();
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    CPU_SET( 0, &cpuset );
    (void) sched_setaffinity( tid, sizeof(cpuset), &cpuset );
  }
  while( 1 ) {
    sem_wait( &sem[0] );
    sem_post( &sem[1] );
  }
}

void *bar( void *args ) {
  {
    pid_t tid = pip_gettid();
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    CPU_SET( 0, &cpuset );
    (void) sched_setaffinity( tid, sizeof(cpuset), &cpuset );
  }
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

  {
    pid_t tid = pip_gettid();
    cpu_set_t cpuset;
    CPU_ZERO( &cpuset );
    CPU_SET( 0, &cpuset );
    (void) sched_setaffinity( tid, sizeof(cpuset), &cpuset );
  }
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
  double dn = (double) niters;
  double min0 = t0[0];
  double min1 = t1[0];
  int idx0 = 0;
  int idx1 = 0;
  for( j=0; j<NSAMPLES; j++ ) {
    printf( "[%d] semaphore : %g  (%lu)\n",
	    j, t0[j] / dn / 2.0,  c0[j] / niters / 2 );
    printf( "[%d] mutex     : %g  (%lu)\n",
	    j, t1[j] / dn / 2.0 , c1[j] / niters / 2 );
    if( min0 > t0[j] ) {
      min0 = t0[j];
      idx0 = j;
    }
    if( min1 > t1[j] ) {
      min1 = t1[j];
      idx1 = j;
    }
  }
  printf( "[[%d]] semaphore : %.3g  (%lu)\n",
	  idx0, t0[idx0] / dn / 2.0, c0[idx0] / niters / 2 );
  printf( "[[%d]] mutex     : %.3g  (%lu)\n",
	  idx1, t1[idx1] / dn / 2.0, c1[idx1] / niters / 2 );
  return 0;
}
