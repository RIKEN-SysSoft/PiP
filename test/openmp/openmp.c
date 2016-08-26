/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>
#include <omp.h>

void omp_loop( void ) {
  char *tag;
  int pipid;
  int nth;
  int tid;

  (void) pip_get_pipid( &pipid );
  if( pipid == PIP_PIPID_ROOT ) {
    tag = "ROOT";
  } else {
    tag = "CHILD";
  }
#pragma omp parallel private(nth, tid)
  {
    /* Obtain thread number */
    nth = omp_get_num_threads();
    tid = omp_get_thread_num();

#ifdef DEBUG
    DBGF( "[%s] Hello World from thread = %d/%d", tag, tid, nth );
#else
    printf( "[%s] Hello World from thread = %d/%d\n", tag, tid, nth );
#endif
  }
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid ) );

    omp_loop();

    TESTINT( pip_wait( pipid, NULL ) );
    TESTINT( pip_fin() );

  } else {

    omp_loop();

  }
  return 0;
}
