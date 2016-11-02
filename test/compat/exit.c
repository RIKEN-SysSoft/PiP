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
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    watch_sigchld();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    sleep( 1 );
    printf( "ROOT: waiting...\n" );
    TESTINT( pip_wait( 0, NULL ) );
    printf( "ROOT: done\n" );

  } else {
    printf( "CHILD: I am going to call exit()\n" );
    exit( 0 );
  }
  TESTINT( pip_fin() );
  return 0;
}
