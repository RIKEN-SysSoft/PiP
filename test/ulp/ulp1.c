/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define NULPS	(10)

#define DEBUG
#include <test.h>

pip_ulp_t ulp_old;

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  void *expp;
  pip_ulp_t ulp_new, *ulp_bakp;

  fprintf( stderr, "PID %d\n", getpid() );

  ntasks = 20;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );
  } else if( pipid == 0 ) {
    fprintf( stderr, "<%d> Hello, I am a parent task !!\n", pipid );
    TESTINT( pip_export( &ulp_old ) );
    pipid ++;
    TESTINT( pip_ulp_create( argv[0], argv, NULL, &pipid, NULL, NULL, &ulp_new ));
    TESTINT( pip_make_ulp( PIP_PIPID_MYSELF, NULL, NULL, &ulp_old ) );
    TESTINT( pip_ulp_yield_to( &ulp_old, &ulp_new ) );
  } else if( pipid < NULPS ) {
    fprintf( stderr, "<%d> Hello, I am a ULP task !!\n", pipid );
    TESTINT( pip_export( &ulp_old ) );
    TESTINT( pip_import( pipid - 1, &expp ) );
    pipid ++;
    TESTINT( pip_ulp_create( argv[0], argv, NULL, &pipid, NULL, NULL, &ulp_new ));
    TESTINT( pip_make_ulp( PIP_PIPID_MYSELF, NULL, NULL, &ulp_old ) );
    TESTINT( pip_ulp_yield_to( &ulp_old, &ulp_new ) );
    ulp_bakp = (pip_ulp_t*) expp;
    TESTINT( pip_ulp_yield_to( NULL, ulp_bakp ) );
  } else {
    fprintf( stderr, "<%d> Hello, I am the last ULP task !!\n", pipid );
    TESTINT( pip_ulp_yield_to( NULL, &ulp_new ) );
  }
  TESTINT( pip_fin() );
  return 0;
}
