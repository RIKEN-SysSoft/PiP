#include <stdio.h>
#include <pip.h>

int main( int argc, char **argv ) {
  int pipid, tid;

  pip_init( &pipid, NULL, NULL, 0 );
  printf( "<%d> %s\n", pipid, pip_get_mode_str() );
#pragma omp parallel private(tid)
  {
    tid = omp_get_thread_num();
    printf( "Hello from PiP-OMP thread %d:%d\n", pipid, tid );
  }
  pip_fin();
  return 0;
}
