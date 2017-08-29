#include <stdio.h>
#include <pip.h>

int gvar = 12345;

#define N	(5)

pip_barrier_t barrier = PIP_BARRIER_INIT(5);

int main( int argc, char **argv ) {
  pip_barrier_t *barrp;
  int pipid, ntasks;

  pip_init( &pipid, &ntasks, NULL, 0 );
  if( ntasks != N ) {
    printf( "Number of PiP tasks must be %d\n", N );
    return 9;
  }

  gvar = pipid;
  pip_get_addr( 0, "barrier", (void**) &barrp );
  pip_barrier_wait( barrp );
  printf( "[%d] gvar=%d\n", pipid, gvar );
  pip_fin();
  return 0;
}
