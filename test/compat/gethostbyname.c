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
  print_fs_segreg();
  CHECK_CTYPE;
  return gethostbyname( name );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  struct hostent *he;

  CHECK_CTYPE;
  print_fs_segreg();
  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    if( ( he = gethostbyname( "localhost" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname( localhost ) fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
    if( ( he = gethostbyname( "127.0.0.1" ) ) == NULL ) {
      fprintf( stderr, "gethostbyname( 127.0.0.1 ) fails\n" );
    } else {
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
    print_maps();
  CHECK_CTYPE;

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    //attachme();
    //print_maps();
  CHECK_CTYPE;

    if( ( he = gethostbyname( "127.0.0.1" ) ) == NULL ) {
      DBG;
      fprintf( stderr, "gethostbyname( 127.0.0.1 ) fails\n" );
    } else {
      DBG;
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
  CHECK_CTYPE;
    if( ( he = my_gethostbyname( "localhost" ) ) == NULL ) {
      DBG;
      fprintf( stderr, "gethostbyname( localhost ) fails\n" );
    } else {
      DBG;
      fprintf( stderr, "hostname: %s\n", he->h_name );
    }
  }
  return 0;
}
