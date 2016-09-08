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
#include <netdb.h>

#define HOSTNAMELEN	(256)

void test_gethostname( void ) {
  char hostnam[HOSTNAMELEN];

  if( gethostname( hostnam, HOSTNAMELEN ) != 0 ) {
    fprintf( stderr, "gethostname()=%d\n", errno );
  } else {
    fprintf( stderr, "hostname:%s\n", hostnam );
  }
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  test_gethostname();

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    test_gethostname();
  }
  return 0;
}
