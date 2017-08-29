#include <stdio.h>
#include <pip.h>
#include "pmap.h"

int gvar = 12345;

#define N	(5)

pip_barrier_t barrier = PIP_BARRIER_INIT(N);

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
  printf( "<%d> 1st: gvar=%d @%p\n", pipid, gvar, &gvar );
  pip_barrier_wait( barrp );
  {
    int *gvarp;
    pip_get_addr( 0, "gvar", (void**) &gvarp );
    *gvarp = 1000 + pipid;
    pip_barrier_wait( barrp );
    printf( "<%d> 2nd: gvar=%d  gvarp=%p\n",
	    pipid, gvar, gvarp );
  }
  if( pipid == 0 ) print_maps();
  pip_fin();
  return 0;
}
