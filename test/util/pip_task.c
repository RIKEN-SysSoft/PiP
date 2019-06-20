/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  int ntasks, pipid, rv, status, extval;
  int i, err;
  char *prog = basename( argv[0] );
  char env[128];

  if( argc < 3 ) return EXIT_UNTESTED;

  ntasks = strtol( argv[1], NULL, 10 );
  rv = pip_init( &pipid, &ntasks, NULL, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "[%s] pip_init: %s\n", prog, strerror( rv ) );
    return EXIT_UNTESTED;
  }
  sprintf( env, "%s=%d", PIP_TASK_NUM_ENV, ntasks );
  putenv( env );

  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    rv = pip_spawn( argv[2], &argv[2], NULL, PIP_CPUCORE_ASIS, &pipid,
		    NULL, NULL, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "[%s] pip_spawn: %s\n", prog, strerror( rv ) );
      return EXIT_UNTESTED;
    }
    if( pipid != i ) {
      fprintf( stderr, "[%s] pip_spawn: pipid %d != %d\n", prog, i, pipid );
      return EXIT_UNTESTED;
    }
  }
  extval = 0;
  err = 0;
  for( i=0; i<ntasks; i++ ) {
    status = 0;
    rv = pip_wait( i, &status );
    if( rv != 0 ) {
      fprintf( stderr, "[%s] pip_wait: %s\n", prog, strerror( rv ) );
      return EXIT_UNTESTED;
    }
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
    } else {
      extval = EXIT_UNRESOLVED;
    }
    if( err == 0 && extval != 0 ) err = extval;
  }

  rv = pip_fin();
  if( rv != 0 ) {
    fprintf( stderr, "[%s] pip_fin: %s\n", prog, strerror( rv ) );
    return EXIT_UNTESTED;
  }

  return extval;
}
