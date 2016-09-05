/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <fcntl.h>

//#define DEBUG
#define PIP_INTERNAL_FUNCS
#include <test.h>

#define ENVNAM		"HOOK_FD"
#define FDNUM		(100)

int bhook( void *hook_arg ) {
  fprintf( stderr, "before hook is called (hook_arg=%d)\n", *(int*) hook_arg );
  return 0;
}

int ahook( void *hook_arg ) {
  fprintf( stderr, "after hook is called (hook_arg=%d)\n", *(int*) hook_arg );
  return 0;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int hook_arg;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    hook_arg = 12345;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) &hook_arg ) );
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
