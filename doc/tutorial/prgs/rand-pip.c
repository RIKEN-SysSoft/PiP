#include <stdlib.h>
#include <stdio.h>
#include <pip.h>

#ifdef USE_PIP_BARRIER
pip_barrier_t barrier, *barrp;
#else
#include <pthread.h>
pthread_barrier_t barrier, *barrp;
#endif

int main( int argc, char **argv ) {
  int i, pipid, seed = 1;
  int ntasks = 100;

  pip_init( &pipid, &ntasks, NULL, 0 );
  if( pipid == 0 ) {
#ifdef USE_PIP_BARRIER
    printf( "PIP BARRIER\n" );
    pip_barrier_init( &barrier, ntasks );
#else
    printf( "PTHREAD BARRIER\n" );
    pthread_barrier_init( &barrier, NULL, ntasks );
#endif
    pip_export( &barrier );
    barrp = &barrier;
  } else {
    do {
      pip_import( 0, (void**) &barrp );
    } while( barrp == NULL );
  }
  srand(1);
  for( i=0; i<4; i++ ) {
#ifdef USE_PIP_BARRIER
    pip_barrier_wait( barrp );
#else
    pthread_barrier_wait( barrp );
#endif
    printf( "<%d> %d : %d\n", pipid, rand(), rand_r(&seed) );
    fflush( NULL );
  }
  pip_fin();
  return 0;
}
