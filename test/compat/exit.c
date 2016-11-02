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

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    int status;

    watch_sigchld();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    sleep( 1 );
    printf( "ROOT: waiting...\n" );
    TESTINT( pip_wait( 0, &status ) );
    printf( "ROOT: done (%d)\n", status );

  } else {
    printf( "CHILD: I am going to call exit(123)\n" );
    exit( 123 );
  }
  TESTINT( pip_fin() );
  return 0;
}
