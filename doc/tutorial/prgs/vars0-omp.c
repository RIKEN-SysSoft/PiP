#include <stdio.h>
#include <omp.h>

int gvar = 0;

int main( int argc, char **argv ) {
  int lvar = 0;
  int tid;

#pragma omp parallel private(tid)
  {
    tid = omp_get_thread_num();
    printf( "<%d> gvar:%p lvar:%p tid:%p\n",
	    tid, &gvar, &lvar );
  }
  return 0;
}
