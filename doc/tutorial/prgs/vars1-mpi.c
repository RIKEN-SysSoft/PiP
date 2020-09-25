#include <stdio.h>
#include <mpi.h>

int gvar = 12345;

int main( int argc, char **argv ) {
  int rank;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  gvar = rank;
  MPI_Barrier( MPI_COMM_WORLD );
  printf( "[%d] gvar=%d\n", rank, gvar );
  MPI_Finalize();
  return 0;
}
