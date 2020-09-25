#include <stdio.h>
#include <omp.h>

int gvar = 12345;

int main( int argc, char **argv ) {

#pragma omp parallel
  {
    gvar = omp_get_thread_num();
#pragma omp barrier
    printf( "[%d] gvar=%d\n",
	    omp_get_thread_num(), gvar );
  }
  return 0;
}
