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

#define NTHREADS	(100)

#define NITERS		(10*1000*1000)

static pthread_t 		threads[NTHREADS];
static int			ids[NTHREADS];
static int 			nthreads;
static pthread_barrier_t	barr;
static pip_atomic_t		count;

static void *thread_main( void *argp ) {
  int id, i, inc, x, y;
  int ninter = 0;

  id = *((int*)argp);
  //fprintf( stderr, "ID:%d\n", id );
  CHECK( (id<0 || id>nthreads), RV, exit(EXIT_FAIL) );

  CHECK( pthread_barrier_wait( &barr ),
	 ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	 exit(EXIT_FAIL) );
  inc = id + 20;
  for( i=0; i<NITERS; i++ ) {
    x = pip_atomic_fetch_and_add( &count, inc );
    y = pip_atomic_sub_and_fetch( &count, inc );
    if( x != y ) ninter ++;
  }
  CHECK( pthread_barrier_wait( &barr ),
	 ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	 exit(EXIT_FAIL) );
  for( i=0; i<nthreads; i++ ) {
    if( i == id ) {
      float percent = ((float)ninter) / ((float)NITERS);
      printf( "[%d] interleaved:%d/%d (%g %%)\n",
	      id, ninter, NITERS, percent*100.0 );
      fflush( NULL );
    }
    CHECK( pthread_barrier_wait( &barr ),
	   ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	   exit(EXIT_FAIL) );
  }
  return NULL;
}

int main( int argc, char **argv ) {
  int i, c;

  nthreads = 10;
  if( argc > 1 ) {
    nthreads = strtol( argv[1], NULL, 10 );
  }
  nthreads = ( nthreads == 0       ) ? NTHREADS : nthreads;
  nthreads = ( nthreads > NTHREADS ) ? NTHREADS : nthreads;

  CHECK( pthread_barrier_init( &barr, NULL, nthreads+1 ),
	 RV,
	 return(EXIT_FAIL) );

  for( i=0; i<nthreads; i++ ) {
    ids[i] = i;
    CHECK( pthread_create( &threads[i], NULL, thread_main, &ids[i] ),
	   RV, return(EXIT_FAIL) );
  }
  CHECK( pthread_barrier_wait( &barr ),
	 ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	 exit(EXIT_FAIL) );
  CHECK( pthread_barrier_wait( &barr ),
	 ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	 exit(EXIT_FAIL) );
  c = count;
  for( i=0; i<nthreads; i++ ) {
    CHECK( pthread_barrier_wait( &barr ),
	   ( RV!=PTHREAD_BARRIER_SERIAL_THREAD && RV!=0 ),
	   exit(EXIT_FAIL) );
  }
  for( i=0; i<nthreads; i++ ) {
    CHECK( pthread_join( threads[i], NULL ), RV, return(EXIT_FAIL) );
  }
  fprintf( stderr, "COUNT: %d\n", c );
  CHECK( (c!=0), RV, return(EXIT_FAIL) );
  return 0;
}
