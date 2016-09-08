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
#include <dlfcn.h>

#define HOSTNAMELEN	(256)

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    void *handle = dlopen( NULL, RTLD_NOW );
    void *mainf;
    if( handle != NULL ) {
      mainf = dlsym( handle, "main" );
      fprintf( stderr, "MAIN function is at %p\n", mainf );
    }
    dlclose( handle );
  }
  return 0;
}
