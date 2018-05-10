#include <stdlib.h>
#include <stdio.h>
#include <omp.h>

int main( int argc, char **argv ) {
  int tid, seed;
  int i;

  srand(1);
#pragma omp parallel private(tid,seed,i)
  {
    seed = 1;
    tid = omp_get_thread_num();
    for( i=0; i<4; i++ ) {
#pragma omp barrier
      printf( "<%d> %d : %d\n", tid, rand(), rand_r(&seed) );
    }
  }
  return 0;
}
