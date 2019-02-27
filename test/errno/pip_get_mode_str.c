/*
 * $RIKEN_copyright$
 * $PIP_VERSION$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  const char *mode_str;

  mode_str = pip_get_mode_str();
  if( mode_str != NULL ) {
      fprintf( stderr, "pip_get_mode: %s", mode_str );
      return EXIT_FAIL;
  }

  return EXIT_PASS;
}
