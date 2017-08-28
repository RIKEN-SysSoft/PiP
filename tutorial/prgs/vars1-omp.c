#include <stdio.h>
#include <omp.h>

int gvar = 0;

int main( int argc, char **argv ) {

#pragma omp parallel
  {
    gvar = omp_get_thread_num();
  }

#pragma omp parallel
  {
    printf( "[%d] gvar=%d\n", omp_get_thread_num(), gvar );
  }
  return 0;
}
