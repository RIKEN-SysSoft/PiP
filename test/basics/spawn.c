/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define DEBUG
#include <test.h>

int main( int argc, char **argv ) {
  int pipid;

  DBG;
  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    ignore_signal( SIGINT );

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );

  } else if( pipid < 10 ) {
    DBGF( "pipid=%d", pipid );
    sleep( 2 );
    set_sigsegv_watcher();
    set_sigint_watcher();

    pipid++;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( pipid, NULL ) );
  }
  DBG;
  TESTINT( pip_fin() );
  DBG;
  return 0;
}
