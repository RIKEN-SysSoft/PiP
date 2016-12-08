/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <unistd.h>
#include <errno.h>

#include <test.h>
#include <string.h>

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  pid_t pid;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    printf( "MAIN\n" );
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    printf( "CHILD done\n" );
    TESTINT( pip_fin() );

  } else {
    if( ( pid = fork() ) == 0 ) {
      print_maps();
      printf( "CHILD-FORK\n" );
    } else {
      printf( "CHILD\n" );
      if( pid < 0 ) {
	printf( "fork() failed (%d)\n", errno );
      } else {
	wait( NULL );
	printf( "CHILD-FORK done\n" );
      }
    }
  }
  return 0;
}
