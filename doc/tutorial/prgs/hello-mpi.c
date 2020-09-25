#include <stdio.h>
#include <mpi.h>

int main( int argc, char **argv ) {
  int rank;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  printf( "Hello from MPI process %d", rank );
  MPI_Finalize();
  return 0;
}
