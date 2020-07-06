#include <stdio.h>
#include <stdlib.h>
#define PIP_EXPERIMENTAL
#include <pip.h>

#define PIP
#ifdef PIP
#define MALLOC(S)	pip_malloc(S)
#define FREE(P)		pip_free(P)
#else
#define MALLOC(S)	malloc(S)
#define FREE(P)		free(P)
#endif

pip_barrier_t barrier = PIP_BARRIER_INIT(2);
void *mem;

int main( int argc, char **argv ) {
  pip_barrier_t *barp = &barrier;
  int pipid;

  pip_init( &pipid, NULL, (void**) &barp, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    pip_spawn( argv[0], argv, NULL, 0, &pipid, NULL, NULL, NULL );
    mem = MALLOC( 128 );
    pip_barrier_wait( barp );
    pip_wait( pipid, NULL );
  } else {
    void **memp;
    pip_barrier_wait( barp );
    pip_get_addr( PIP_PIPID_ROOT, "mem", (void**) &memp );
    FREE( *memp );
  }
  pip_fin();
  return 0;
}
