#include <stdio.h>
#include <pip.h>

int main( int argc, char **argv ) {
  int pipid;

  pip_init( &pipid, NULL, NULL, 0 );
  printf( "Hello from PiP task %d\n", pipid );
  pip_fin();
  return 0;
}
