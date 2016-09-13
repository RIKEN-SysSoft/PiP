/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define DEBUG
#include <test.h>
#include <netdb.h>

struct hostent *my_gethostbyname( char *name ) {
  DBG;
  return gethostbyname( name );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  struct hostent *he;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
#ifdef AH
    if( ( he = gethostbyname( "localhost" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname() fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
#endif
    print_maps();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    //attachme();
    if( ( he = my_gethostbyname( "localhost" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname() fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
  }
  return 0;
}
