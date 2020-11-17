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

int main( int argc, char **argv ) {
  pip_barrier_t	barr, *barrp;
  int 	ntasks, pipid;
  int 	niters = 0, i;
  volatile int	count, *countp;

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters <= 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ),		       RV, return(EXIT_FAIL) );
  if( pipid == 0 ) {
    CHECK( pip_barrier_init( &barr, ntasks ), 		       RV, return(EXIT_FAIL) );
    barrp = &barr;
    CHECK( pip_named_export( (void*) barrp,  "BARRIER" ),      RV, return(EXIT_FAIL) );
    count = 0;
    countp = &count;
    CHECK( pip_named_export( (void*) countp, "COUNT" ),        RV, return(EXIT_FAIL) );
  } else {
    barrp = NULL;
    CHECK( pip_named_import( 0, (void**) &barrp,  "BARRIER" ), RV, return(EXIT_FAIL) );
    CHECK( barrp==NULL,					       RV, return(EXIT_FAIL) );
    countp = NULL;
    CHECK( pip_named_import( 0, (void**) &countp, "COUNT" ),   RV, return(EXIT_FAIL) );
    CHECK( countp==NULL,			               RV, return(EXIT_FAIL) );
  }
  for( i=0; i<niters; i++ ) {
    CHECK( *countp!=i,                 			       RV, return(EXIT_FAIL) );
    CHECK( pip_barrier_wait( barrp ), 			       RV, return(EXIT_FAIL) );
    if( ( i % ntasks ) == pipid ) (*countp) ++;
    CHECK( pip_barrier_wait( barrp ), 			       RV, return(EXIT_FAIL) );
  }
  if( pipid == 0 ) {
    CHECK( count==niters,   				      !RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), 					       RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
