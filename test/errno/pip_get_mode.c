/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 1.2.0$
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
