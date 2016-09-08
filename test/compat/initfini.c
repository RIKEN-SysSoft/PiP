/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#define DEBUG
#include <test.h>

#define PROGNAM		"./iftask"

int root_exp = 0;

int main( int argc, char **argv ) {
  char *newav[2];
  int pipid, ntasks;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    newav[0] = PROGNAM;
    newav[1] = NULL;
    pipid = 0;
    TESTINT( pip_spawn( newav[0], newav, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    /* should not reach here */
    fprintf( stderr, "ROOT: NOT ROOT ????\n" );
  }
  return 0;
}
