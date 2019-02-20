/*
 * $RIKEN_copyright$
 * $PIP_VERSION$
 * $PIP_license$
 */

#include <test.h>

int main( int argc, char **argv ) {
  int pipid, ntasks, rv;
  void *exp;

  ntasks = 1;
  exp = NULL;
  rv = pip_init( &pipid, &ntasks, &exp, 0);

  
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    rv = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		    NULL, NULL, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_spawn: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_fin();
    if( rv != EBUSY ) {
      fprintf( stderr, "pip_fin/1: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_wait( 0, NULL );
    if( rv != 0 ) {
      fprintf( stderr, "pip_wait: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    rv = pip_fin();
    if( rv != 0 ) {
      fprintf( stderr, "pip_fin/2: %s", strerror( rv ) );
      return EXIT_FAIL;
    }
    printf( "Hello, pip root task is fine !!\n" );
  } else {
    printf( "<%d> sleeping...\n", pipid );
    sleep(1);
    printf( "<%d> slept!!!\n", pipid );
  }

  return EXIT_PASS;
}
