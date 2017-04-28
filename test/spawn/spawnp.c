/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#include <test.h>

int main( int argc, char **argv ) {
  int pipid, ntasks;

  ntasks = 10;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    fprintf( stderr, "[PID=%d] Hello, I am ROOT!\n", getpid() );
    pipid = 0;
    TESTINT( pip_spawn( argv[0],
			argv,
			NULL,
			PIP_CPUCORE_ASIS,
			&pipid ,
			NULL,
			NULL,
			NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );
  } else if( pipid < ntasks ) {
    fprintf( stderr, "[PID=%d] Hello, PIPID=%d\n", getpid(), pipid );
    pipid ++;
    TESTINT( pip_spawn( argv[0],
			argv,
			NULL,
			PIP_CPUCORE_ASIS,
			&pipid ,
			NULL,
			NULL,
			NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );
  }
  TESTINT( pip_fin() );
  return 0;
}
