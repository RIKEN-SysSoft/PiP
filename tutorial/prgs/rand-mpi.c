#include <stdio.h>
#include <mpi.h>

int main( int argc, char **argv ) {
  int i, rank, seed=1;

  MPI_Init( &argc, &argv );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  srand(1);
  for( i=0; i<4; i++ ) {
    MPI_Barrier( MPI_COMM_WORLD );
    printf( "<%d> %d : %d\n", rank, rand(), rand_r(&seed) );
    fflush( NULL );
    sleep( 1 );
  }
  MPI_Finalize();
  return 0;
}
