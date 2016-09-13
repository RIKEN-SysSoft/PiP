/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <test.h>

int main( int argc, char **argv ) {
  int pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    ignore_signal( SIGINT );

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );
    TESTINT( pip_fin() );

  } else if( pipid < 10 ) {
    DBGF( "pipid=%d", pipid );

    set_sigsegv_watcher();
    set_sigint_watcher();

    pipid = PIP_PIPID_ANY;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );
  }
  DBG;
  return 0;
}
