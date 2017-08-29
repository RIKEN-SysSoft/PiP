#include <stdio.h>
#include <mpi.h>
#include "pmap.h"

int gvar = 0;

int main( int argc, char **argv ) {
  int rank, size, i;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  gvar = rank;
  MPI_Barrier( MPI_COMM_WORLD );
  printf( "<%d> gvar=%d @%p\n", rank, gvar, &gvar );
  fflush( stdout );
  MPI_Barrier( MPI_COMM_WORLD );
  for( i=0; i<size; i++ ) {
    if( i == rank ) print_maps();
    MPI_Barrier( MPI_COMM_WORLD );
  }
  MPI_Finalize();
  return 0;
}
