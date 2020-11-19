/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 1.2.0$
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
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define NTHREADS	(100)

//#define DEBUG

#define PIP_INTERNAL_FUNCS
#include <test.h>

void *thread( void *argp ) {
  // TESTINT( pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, NULL ) );
  // the PTHREAD_CANCEL_DISABLE helps nothing
  // pthread_exit( NULL );  /* pthread_exit() does SOMETHING WRONG !!!! */
  return NULL;
}

int main( int argc, char **argv ) {
  int pipid    = 999;
  int ntasks   = NTASKS;
  int nthreads = NTHREADS;
  int i;
  int err;

  DBG;
  if( !pip_isa_piptask() ) {
    if( argc     >  1 ) ntasks = atoi( argv[1] );
    if( ntasks   <= 0 ||
	ntasks   >  NTASKS ) ntasks = NTASKS;
  } else {
    if( argc      > 2 ) nthreads = atoi( argv[2] );
    if( nthreads <= 0 ||
	nthreads >  NTHREADS ) nthreads = NTHREADS;
  }

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, ntasks, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
//#define SERIAL
#ifndef SERIAL
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
#endif
      TESTINT( pip_wait( i, NULL ) );
    }

  } else {
    pthread_t threads[NTHREADS];
    for( i=0; i<nthreads; i++ ) {
      TESTINT( pthread_create( &threads[i],NULL, thread, NULL ) );
#ifndef SERIAL
    }
    DBG;
    for( i=0; i<nthreads; i++ ) {
#endif
      //while( pthread_tryjoin_np( threads[i], NULL ) != 0 );
      TESTINT( pthread_join( threads[i], NULL ) );
      printf( "SUCCESS\n" );
    }
  }
  TESTINT( pip_fin() );
  DBG;
  return 0;
}
