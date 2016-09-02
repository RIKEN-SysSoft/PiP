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

int bhook( char ***argvp, char ***envvp ) {
  char fdstr[32];
  char **envv;
  int fd;

  if( ( fd = dup2( 1, FDNUM ) ) < 0 ) return errno;
  sprintf( fdstr, "%s=%d", ENVNAM, fd );

  fprintf( stderr, "before hook is called (%s)\n", fdstr );

  envv = pip_copy_vec( fdstr, *envvp );
  if( envv == NULL ) return ENOMEM;
  *envvp = envv;
  return 0;
}

int ahook( char ***argvp, char ***envvp ) {
  fprintf( stderr, "after hook is called\n" );
  free( *envvp );
  return 0;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook) );
    TESTINT( pip_wait( 0, NULL ) );

    char *mesg = "This message should not apper\n";
    if( write( FDNUM, mesg, strlen( mesg ) ) > 0 ) {
      fprintf( stderr, "something wrong !!\n" );
    }
    TESTINT( pip_fin() );

  } else {
    char mesg[128];
    char *env;
    int fd;

    if( ( env = getenv( ENVNAM ) ) != NULL ) {
      fd = atoi( env );
      sprintf( mesg, "[%d] Hello, I am fine with FD[%d] !!\n", pipid, fd );
      if( write( fd, mesg, strlen( mesg ) ) < 0 ) {
	fprintf( stderr, "write(%d) returns %d (env=%s)\n", fd, errno, env );
      }
    } else {
      fprintf( stderr, "%s not found\n", ENVNAM );
    }
  }
  return 0;
}
