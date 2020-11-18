/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
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

#define NITERS		(100)

#define MAGIC		(12345)


int main( int argc, char **argv ) {
#ifdef BARRIER
  pip_barrier_t barrier;
  pip_barrier_t	*barrp;
#endif
  int 	pipid, ntasks, prev, next;
  int 	niters = 0, i;
  int 	count, *counts, *countp;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL)     );
  CHECK( pipid==PIP_PIPID_ROOT,                RV, return(EXIT_UNTESTED) );
  CHECK( ntasks<2,                             RV, return(EXIT_UNTESTED) );

  prev = pipid - 1;
  prev = ( prev < 0 ) ? ntasks - 1 : prev;
  next = pipid + 1;
  next = ( next == ntasks ) ? 0 : next;

  CHECK( pip_named_import( pipid, (void**) &countp, "must not be found" ),
	 RV!=EDEADLK, return(EXIT_FAIL) );
  count = MAGIC;
  CHECK( pip_named_export( (void*) &count, "must be found" ),
	 RV, return(EXIT_FAIL) );
  CHECK( pip_named_import( pipid, (void**) &countp, "must be found" ),
	 RV, return(EXIT_FAIL) );
  CHECK( *countp!=MAGIC, RV, return(EXIT_FAIL) );

  CHECK( counts=(int*)malloc(sizeof(int)*niters), RV==0, return(EXIT_UNTESTED) );
  for( i=0; i<niters; i++ ) {
    counts[i] = i + MAGIC;
  }
#ifdef BARRIER
  if( pipid == 0 ) {
    barrp = &barrier;
    CHECK( pip_barrier_init( barrp, ntasks ), RV, return(EXIT_FAIL) );
    CHECK( pip_named_export( (void*) &barrier, "barrier" ),
	   RV, return(EXIT_FAIL) );
  } else {
    CHECK( pip_named_import( 0, (void**) &barrp, "barrier" ),
	   RV, return(EXIT_FAIL) );
  }
  CHECK( pip_barrier_wait( barrp ), RV, return(EXIT_FAIL) );
#endif
  for( i=0; i<niters; i++ ) {
    if( pipid == 0 ) {
      CHECK( pip_named_export( (void*) &counts[i], "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( next, (void**) &countp, "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( prev, (void**) &countp, "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
    } else {
      CHECK( pip_named_import( prev, (void**) &countp, "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "forward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_export( (void*) &counts[i], "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( pip_named_import( next, (void**) &countp, "backward:%d", i ),
	     RV, return(EXIT_FAIL) );
      CHECK( *countp!=i+MAGIC, RV, return(EXIT_FAIL) );
    }
  }
#ifdef BARRIER
  CHECK( pip_barrier_wait( barrp ), RV, return(EXIT_FAIL) );
#endif
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
