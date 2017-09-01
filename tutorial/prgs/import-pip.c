#include <stdio.h>
#include <pip.h>

#define N	(4)

pip_barrier_t barrier = PIP_BARRIER_INIT(N);

int main( int argc, char **argv ) {
  pip_barrier_t *barp = &barrier;
  int ntasks = N, pipid, i;

  pip_init( &pipid, &ntasks, (void**) &barp, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<N; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    for( i=0; i<N; i++ ) pip_wait( pipid, NULL );
  } else {
    pip_barrier_wait( barp );
    printf( "Child: %d\n", pipid );
  }
  pip_fin();
  return 0;
}
