/*
 * $RIKEN_copyright$
 * $PIP_VERSION$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  int mode, rv;

  rv = pip_get_mode( &mode );
  if( rv != EPERM ) {
      fprintf( stderr, "pip_get_mode: %s", strerror( rv ) );
      return EXIT_FAIL;
  }

  return EXIT_PASS;
}
