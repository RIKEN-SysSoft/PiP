/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>
#include <string.h>
#include <ctype.h>

char *to_upper( char *lower ) {
  char *upper = strdup( lower );
  int i;

  PIP_CHECK_CTYPE;
  for( i=0; upper[i]!='\0'; i++ ) {
    DBGF( "i=%d", i );
    upper[i] = toupper( upper[i] );
  }
  return upper;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  PIP_CHECK_CTYPE;
  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    printf( "MAIN: %s\n", to_upper( "abcdefg" ) );
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    //print_maps();

    printf( "CHILD: %s\n", to_upper( "xyz12345" ) );
  }
  return 0;
}
