/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <test.h>

#define NTHREADS 10
#define NITERS 10

void *thread_main( void *argp ) {
  //pthread_exit(NULL);
  return NULL;
}

int main( int argc, char **argv ) {
  pthread_t threads[NTHREADS];
  int nthreads, niters;
  int i, j;

  nthreads = 0;
  if( argc > 1 ) {
    nthreads = strtol( argv[1], NULL, 10 );
  }
  nthreads = ( nthreads <= 0       ) ? NTHREADS : nthreads;
  nthreads = ( nthreads > NTHREADS ) ? NTHREADS : nthreads;

  niters = 0;
  if( argc > 2 ) {
    niters = strtol( argv[2], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  for( i=0; i<niters; i++ ) {
    for( j=0; j<nthreads; j++ ) {
      CHECK( pthread_create( &threads[j], NULL, thread_main, NULL ),
	     RV, return(EXIT_FAIL) );
    }
    for( j=0; j<nthreads; j++ ) {
      CHECK( pthread_join( threads[j], NULL ), RV, return(EXIT_FAIL) );
    }
  }
  return 0;
}
