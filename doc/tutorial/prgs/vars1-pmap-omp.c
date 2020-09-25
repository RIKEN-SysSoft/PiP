#include <stdio.h>
#include <omp.h>
#include "pmap.h"

int gvar = 12345;

int main( int argc, char **argv ) {
  int t, n, i = 0;
#pragma omp parallel private(t,n,i)
  {
    t = omp_get_thread_num();
    n = omp_get_num_threads();
    gvar = t;
#pragma omp barrier
    printf( "<%d> gvar=%d @%p\n",
	    omp_get_thread_num(), gvar, &gvar );
    fflush( stdout );
#pragma omp barrier
    for( i=0; i<n; i++ ) {
      if( i == t ) print_maps();
#pragma omp barrier
    }
  }
  return 0;
}
