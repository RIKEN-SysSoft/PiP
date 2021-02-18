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
 * System Software Development Team, 2016-2021
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS)
 * Query:   procinproc-info@googlegroups.com
 * User ML: procinproc-users@googlegroups.com
 * $
 */

#include <pip/pip.h>

/* Although PiP provides the barrier feature */
/* pthread_barrier also works */
#ifdef PTHREAD_BARRIER
#define BARRIER_T		pthread_barrier_t
#define BARRIER_INIT(B,C)	pthread_barrier_init(B,NULL,C)
#define BARRIER_WAIT		pthread_barrier_wait
#else
#define BARRIER_T		pip_barrier_t
#define BARRIER_INIT(B,C)	pip_barrier_init(B,C)
#define BARRIER_WAIT		pip_barrier_wait
#endif

#define NDATA		1000000

struct dat {
  BARRIER_T	barrier;
  float	data[NDATA];
} data;

float output = 0.0;

int main( int argc, char **argv ) {
  void *export = (void*) &data;
  int  i, ntasks, pipid;

  ntasks = 8;
  pip_init( &pipid, &ntasks, (void*) &export, 0 );
  if( pipid == PIP_PIPID_ROOT ) {
    /* initialize */
    BARRIER_INIT( &data.barrier, ntasks + 1 );
    for( i=0; i<NDATA;  i++ ) data.data[i] = (float) i;
    /* spawning PiP tasks */
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    /* wait for the local summations */
    BARRIER_WAIT( &data.barrier );
    /* gather individual result */
    for( i=0; i<ntasks; i++ ) {
      void *import;
      pip_named_import( i, &import, "data" );
      output += *((float*)(import));
    }
    BARRIER_WAIT( &data.barrier );
    /* wait for PiP task terminations */
    for( i=0; i<ntasks; i++ ) pip_wait( i, NULL );
    printf( "Grand total = %g\n", output );

  } else {	/* PIP child task */
    struct dat* import = (struct dat*) export;
    float *input = import->data;
    int start, end;

    start = ( NDATA / ntasks ) * pipid;
    end = start + ( NDATA / ntasks );
    printf( "PIPID:%d  data[%d-%d]\n", pipid, start, end-1 );
    /* do computation on imported data */
    for( i=start; i<end; i++ ) {
      output += input[i];
    }
    /* export the local sum */
    pip_named_export( (void*) &output, "data" );

    BARRIER_WAIT( &import->barrier );
    /* here, the main task gathers child data */
    BARRIER_WAIT( &import->barrier );
  }
  pip_fin();
  return 0;
}
