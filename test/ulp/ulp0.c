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

#ifdef AHAH
#define AH

#define DEBUG
#include <test.h>

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  pip_ulp_t ulp;

  fprintf( stderr, "PID %d\n", getpid() );

  ntasks = 20;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
#ifdef AH
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );
  } else if( pipid == 0 ) {

    fprintf( stderr, "<%d> Hello, I am a parent task !!\n", pipid );
    pipid ++;
    TESTINT( pip_ulp_spawn( argv[0], argv, NULL, &pipid, NULL, &ulp ) );
#else
    TESTINT( pip_ulp_spawn( argv[0], argv, NULL, &pipid, NULL, &ulp ) );
#endif
  } else if( pipid < NULPS ) {
    fprintf( stderr, "<%d> Hello, I am a ULP task !!\n", pipid );
    pipid ++;
    TESTINT( pip_ulp_spawn( argv[0], argv, NULL, &pipid, NULL, &ulp ) );
  } else {
    fprintf( stderr, "<%d> Hello, I am the last ULP task !!\n", pipid );
  }
  TESTINT( pip_fin() );
  return 0;
}

#else

int main() { return 0; }

#endif
