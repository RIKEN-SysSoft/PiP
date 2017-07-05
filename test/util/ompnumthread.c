/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */

#include <omp.h>
#include <stdio.h>

int nth;

int main() {
#pragma omp parallel
  {
    nth = omp_get_num_threads();
  }
  printf( "%d\n", nth );
  return 0;
}
