#include <stdio.h>
#include <pip.h>

int main( int argc, char **argv ) {
  int pipid;

  pip_init( &pipid, NULL, NULL, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    printf( "Root\n" );		/* PiP Root */
    pipid = 9;
    pip_spawn( argv[0], argv, NULL, 0, &pipid, NULL, NULL, NULL );
    pip_wait( pipid, NULL );
  } else {			/* PiP child task */
    printf( "Child: %d\n", pipid );
  }
  pip_fin();
  return 0;
}
