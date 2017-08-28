#include <stdio.h>
#include <pip.h>

int gvar = 0;

pip_barrier_t barrier = PIP_BARRIER_INIT(5);

int main( int argc, char **argv ) {
  pip_barrier_t *barrp;
  int pipid;

  pip_init( &pipid, NULL, NULL, 0 );
  gvar = pipid;

  pip_get_addr( 0, "barrier", (void**) &barrp );
  pip_barrier_wait( barrp );
  printf( "[%d] 1st: gvar=%d\n", pipid, gvar );
  pip_barrier_wait( barrp );
  {
    int *gvarp;
    pip_get_addr( 0, "gvar", (void**) &gvarp );
    *gvarp = pipid + 100;
    pip_barrier_wait( barrp );
    if( pipid == 0 ) {
      printf( "[%d] 2nd: gvar=%d\n", pipid, gvar );
    }
  }
  pip_fin();
  return 0;
}
