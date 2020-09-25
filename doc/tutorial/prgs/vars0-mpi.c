#include <stdio.h>
#include <mpi.h>

int gvar = 0;

int main( int argc, char **argv ) {
  int lvar = 0;
  int rank;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  printf( "<%d> gvar:%p lvar:%p\n",
	  rank, &gvar, &lvar );
  MPI_Finalize();
  return 0;
}
