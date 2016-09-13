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

//#define FILENAME	"/home/ahori/work/PIP/GLIBC/glibc-2.17-c758a686/"
//#define FILENAME 	"/usr/lib64/libcurses.so"
#define FILENAME 	argv[0]
#define HOSTNAMELEN	(256)

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int err = 0;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    print_maps();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    void *handle;
    void *mainf;

    //attachme();
    handle = dlopen( FILENAME, RTLD_LAZY );
    if( handle != NULL ) {
      mainf = dlsym( handle, "main" );
      fprintf( stderr, "MAIN function is at %p\n", mainf );
      dlclose( handle );
    } else {
      fprintf( stderr, "dlopen(%s): %s\n", FILENAME, dlerror() );
      err = 1;
    }
  }
  return err;
}
