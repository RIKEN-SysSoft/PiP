/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 1.2.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#include <sys/wait.h>
#include <pthread.h>
#include <stdio.h>
#include <pip.h>

#define NDATA		1000000

struct dat {
  pthread_barrier_t	barrier;
  double		data[NDATA];
} data;

int main( int argc, char **argv ) {
  void *export = (void*) &data;
  double output = 0.0;
  int  i, ntasks, pipid;

  ntasks = 10;
  pthread_barrier_init( &data.barrier, NULL, ntasks + 1 );
  pip_init( &pipid, &ntasks, (void*) &export, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NDATA; i++ ) data.data[i] = (double) i;
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    pthread_barrier_wait( &data.barrier );
    for( i=0; i<ntasks; i++ ) {
      void *import;
      pip_import( i, &import );
      /* gather individual result */
      output += *((double*)(import));
    }
    pthread_barrier_wait( &data.barrier );
    for( i=0; i<ntasks; i++ ) wait( NULL );
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
      output += input[i];;
    }
    /* note that any stack variables can also be exported */
    pip_export( (void*) &output );

    pthread_barrier_wait( &import->barrier );
    /* here, the main task gathers child data */
    pthread_barrier_wait( &import->barrier );
  }
  pip_fin();
  return 0;
}
