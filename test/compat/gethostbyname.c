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

#define HOSTNAMELEN	(256)

#define BUFLEN		(1024)
struct hostent *my_gethostbyname( char *name ) {
  static char *buffer = NULL;
  static struct hostent ret, *res;
  static int len = BUFLEN;
  int herr, err;

  if( buffer == NULL ) {
    buffer = (char*) malloc( len );
    if( buffer == NULL ) return NULL;
  }
  while( 1 ) {
    err = gethostbyname_r( name, &ret, buffer, len, &res, &herr );
    if( err != ERANGE ) break;
    len *= 2;
    if( ( buffer = (char*) realloc( buffer, len ) ) == NULL ) return NULL;
  }
  return res;

}

void test_gethostname( void ) {
  char hostnam[HOSTNAMELEN];
  extern int h_errno;
  struct hostent *hostent;

  DBG;
  if( gethostname( hostnam, HOSTNAMELEN ) != 0 ) {
  DBG;
    fprintf( stderr, "gethostname()=%d\n", errno );
  } else {
  DBG;
    fprintf( stderr, "hostname:%s\n", hostnam );
  DBG;
    if( ( hostent = my_gethostbyname( hostnam ) ) == NULL ) {
  DBG;
      fprintf( stderr, "gethostbyname()=%d\n", h_errno );
    } else {
  DBG;
      fprintf( stderr, "Hello, I am running on %s !!\n", hostnam );
  DBG;
    }
  }
  DBG;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  test_gethostname();

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_PRELOAD_NISLIBS ) );
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
