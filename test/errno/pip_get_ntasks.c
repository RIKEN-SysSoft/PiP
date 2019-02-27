/*
 * $RIKEN_copyright$
 * $PIP_VERSION$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  int ntasks, rv;

  rv = pip_get_ntasks( &ntasks );
  if( rv != EPERM ) {
      fprintf( stderr, "pip_get_ntasks: %s", strerror( rv ) );
      return EXIT_FAIL;
  }

  return EXIT_PASS;
}
