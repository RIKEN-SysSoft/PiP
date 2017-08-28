#include <stdio.h>
#include <pip.h>

int gvar = 0;

int main( int argc, char **argv ) {
  int lvar = 0;
  int pipid;

  pip_init( &pipid, NULL, NULL, 0 );
  printf( "[%d] gvar:%p lvar:%p\n", pipid, &gvar, &lvar );
  pip_fin();
  return 0;
}
