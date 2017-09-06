#include <stdio.h>
#include <pip.h>

#include "pmap.h"

#define N	(3)
pip_barrier_t barrier = PIP_BARRIER_INIT(N);
static int var = 12345;

int foo( void ) { return var; }

int main( int argc, char **argv ) {
  pip_barrier_t *barp = &barrier;
  int ntasks = N, pipid, i;

  pip_init( &pipid, &ntasks, (void**) &barp, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<N; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, 0, &pipid, NULL, NULL, NULL );
    }
    for( i=0; i<N; i++ ) pip_wait( pipid, NULL );
  } else {
    int(*func)(void);
    var = 1000 + pipid * 10;
    pip_barrier_wait( barp );
    if( pipid == 0 ) print_maps();
    for( i=0; i<N; i++ ) {
      pip_get_addr( i, "foo", (void**) &func );
      printf( "<%d> var<%d> = %d\n", pipid, i, func() );
    }
  }
  pip_fin();
  return 0;
}
