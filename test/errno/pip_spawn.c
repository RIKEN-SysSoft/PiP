/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  int pipid, rv, status, extval;

  pipid = PIP_PIPID_ANY;
  rv = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		  NULL, NULL, NULL );
  if( rv != EPERM ) {
      fprintf( stderr, "pip_spawn: %s", strerror( rv ) );
      return EXIT_FAIL;
  }
  statis = 0;
  rv = pip_wait( pipid, &status );
  if( rv != 0 ) {
    fprintf( stderr, "pip_wait: %s", strerror( rv ) );
    return EXIT_FAIL;
  }
  if( WIFEXITED( status ) ) {
    extval = WEXITSTATUS( status );
  } else {
    extval = EXIT_UNRESOLVED;
  }

  rv = pip_fin();
  if( rv != 0 ) {
    fprintf( stderr, "pip_fin: %s", strerror( rv ) );
    return EXIT_FAIL;
  }

  return extval;
}
