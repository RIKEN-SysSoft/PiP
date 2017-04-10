/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

/***
    This test program is to see if the VDSO function call
    may not cause any problem.
 ***/

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>

#include <sys/time.h>

int main( int argc, char **argv ) {
  int pipid = 999;
  int i, ntasks;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		       NULL, NULL, NULL );
      if( err != 0 ) break;
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
    TESTINT( pip_fin() );

  } else {
    system( "hostname" );
  }
  return 0;
}
