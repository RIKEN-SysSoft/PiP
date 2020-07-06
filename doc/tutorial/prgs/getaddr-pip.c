#include <stdio.h>
#include <pip.h>

#define N	(2)

pip_barrier_t barrier = PIP_BARRIER_INIT(N);
int x;

int main( int argc, char **argv ) {
  pip_barrier_t *barp = &barrier;
  int ntasks = N, pipid, i;

  pip_init( &pipid, &ntasks, (void**) &barp, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    x = 12345;
    for( i=0; i<N; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    for( i=0; i<N; i++ ) pip_wait( i, NULL );
  } else {
    int *xp;

    x = pipid * 10 + 1000;
    pip_barrier_wait( barp );
    pip_get_addr( PIP_PIPID_ROOT, "x", (void**) &xp );
    printf( "<%d> x<ROOT>=%d\n", pipid,*xp );
    for( i=0; i<ntasks; i++ ) {
      pip_get_addr( i, "x", (void**) &xp );
      printf( "<%d> x<%d>=%d\n", pipid, i, *xp );
    }
  }
  pip_fin();
  return 0;
}
