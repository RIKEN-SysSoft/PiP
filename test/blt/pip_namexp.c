/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 3.0.0$
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

#include <test.h>

#define NITERS		(1000)

static int count;
static pip_atomic_t done;

static int conv( int x ) {
  return( x * 100 );
}

int main( int argc, char **argv ) {
  int 	ntasks, pipid;
  int 	niters = 0, i;
  int 	*countp;
  pip_atomic_t	*donep;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL)     );
  CHECK( pipid==PIP_PIPID_ROOT,                RV, return(EXIT_UNTESTED) );
  CHECK( ntasks<2,                             RV, return(EXIT_UNTESTED) );

  srand( ( pipid + 1 ) * ( pipid + 1 ) );
  count = conv( pipid );
  CHECK( pip_named_export( (void*) &count, "COUNT:%d", pipid ),
	 RV, return(EXIT_FAIL) );

  for( i=0; i<niters; i++ ) {
    int target;
    do {
      target = rand() % ntasks;
    } while( target == pipid );
    countp = NULL;
    CHECK( pip_named_import( target, (void**) &countp, "COUNT:%d", target ),
	   RV, return(EXIT_FAIL) );
    CHECK( countp==NULL,                 RV,               return(EXIT_FAIL) );
    CHECK( *countp!=conv(target),        RV,               return(EXIT_FAIL) );
    CHECK( pip_yield(PIP_YIELD_DEFAULT), RV!=0&&RV!=EINTR, return(EXIT_FAIL) );
  }

  if( pipid == 0 ) {
    for( i=1; i<ntasks; i++ ) {
      donep = NULL;
      CHECK( pip_named_import( i, (void**) &donep, "DONE" ),
	     RV, return(EXIT_FAIL) );
      CHECK( donep==NULL, RV, return(EXIT_FAIL) );
      CHECK( *donep!=i,   RV, return(EXIT_FAIL) );
      *donep = -1;
    }
    done = 1;
    CHECK( pip_named_export( (void*) &done, "DONE" ), RV,    return(EXIT_FAIL) );
    while( done < ntasks ) {
      CHECK( pip_yield(PIP_YIELD_DEFAULT), RV!=0&&RV!=EINTR, return(EXIT_FAIL) );
    }
  } else {
    done = pipid;
    CHECK( pip_named_export( (void*) &done, "DONE" ), RV,    return(EXIT_FAIL) );
    while( done >= 0 ) {
      CHECK( pip_yield(PIP_YIELD_DEFAULT), RV!=0&&RV!=EINTR, return(EXIT_FAIL) );
    }
    donep = NULL;
    CHECK( pip_named_import( 0, (void**) &donep, "DONE" ),
	   RV, return(EXIT_FAIL) );
    CHECK( donep==NULL, RV, return(EXIT_FAIL) );
    (void) pip_atomic_fetch_and_add( donep, 1 );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
