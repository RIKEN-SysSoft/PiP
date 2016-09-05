/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  watch_sigchld();

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    fprintf( stderr, "<%d> Hello, I am fine !!\n", pipid );
  }
  return 0;
}
