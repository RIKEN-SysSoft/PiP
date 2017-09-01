#include <pip.h>

int main( int argc, char **argv ) {
  int pipid;

  pip_init( &pipid, NULL, NULL, 0 );
  if( pipid == PIP_PIPID_ROOT && argv[1] != NULL ) {
    pipid = 0;
    pip_spawn( argv[1], &argv[1], NULL, PIP_CPUCORE_ASIS, &pipid,
	       NULL, NULL, NULL );
    pip_wait( pipid, NULL );
  }
  pip_fin();
  return 0;
}
