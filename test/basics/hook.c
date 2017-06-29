/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#define PIP_INTERNAL_FUNCS
#include <test.h>

#include <fcntl.h>

#define ENVNAM		"HOOK_FD"
#define FDNUM		(100)

#define SZ	(16)

int bhook( void *hook_arg ) {
  int *array = (int*) hook_arg;
  int i;
  fprintf( stderr, "before hook is called\n" );
  for( i=0; i<SZ; i++ ) {
    if( array[i] != i ) {
      fprintf( stderr, "Hook_arg[%d] != %d\n", i, array[i] );
      return -1;
    }
  }
  return 0;
}

int ahook( void *hook_arg ) {
  fprintf( stderr, "after hook is called\n" );
  free( hook_arg );
  return 0;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int *hook_arg;
  int i;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    hook_arg = (int*) malloc( sizeof(int) * SZ );
    for( i=0; i<SZ; i++ ) hook_arg[i] = i;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) hook_arg ) );
    TESTINT( pip_wait( 0, NULL ) );

    char *mesg = "This message should not apper\n";
    if( write( FDNUM, mesg, strlen( mesg ) ) > 0 ) {
      fprintf( stderr, "something wrong !!\n" );
    }
    TESTINT( pip_fin() );

  } else {
    fprintf( stderr, "[%d] Hello, I am fine !!\n", pipid );
  }
  return 0;
}
