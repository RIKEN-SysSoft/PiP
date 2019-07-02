#include <stdio.h>
#include <pip.h>
#include <pip_blt.h>

#define NDATA		1000000

struct dat {
  pip_barrier_t	barrier;
  double	data[NDATA];
} data;

int main( int argc, char **argv ) {
  void *export = (void*) &data;
  double output = 0.0;
  int  i, ntasks, pipid;

  ntasks = 8;
  pip_barrier_init( &data.barrier, ntasks + 1 );
  pip_init( &pipid, &ntasks, (void*) &export, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NDATA; i++ ) data.data[i] = (double) i;
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    pip_barrier_wait( &data.barrier );
    for( i=0; i<ntasks; i++ ) {
      void *import;
      pip_import( i, &import );
      /* gather individual result */
      output += *((double*)(import));
    }
    pip_barrier_wait( &data.barrier );
    for( i=0; i<ntasks; i++ ) pip_wait( i, NULL );
    printf( "output = %g\n", output );

  } else {	/* PIP child task */
    struct dat* import = (struct dat*) export;
    double *input = import->data;
    int start, end;

    start = ( NDATA / ntasks ) * pipid;
    end = start + ( NDATA / ntasks );
    printf( "PIPID:%d  data[%d-%d]\n", pipid, start, end-1 );
    for( i=start; i<end; i++ ) {
      /* do computation on imported data */
      output += input[i];
    }
    /* note that any stack variables can also be exported */
    pip_export( (void*) &output );

    pip_barrier_wait( &import->barrier );
    /* here, the main task gathers child data */
    pip_barrier_wait( &import->barrier );
  }
  pip_fin();
  return 0;
}
