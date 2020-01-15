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

#define DEBUG
#include <test.h>

#define NITERS		(100)

pip_barrier_t	barr;
pip_mutex_t 	mutex;
volatile int	count;

int main( int argc, char **argv ) {
  pip_barrier_t	*barrp;
  pip_mutex_t 	*mutexp;
  volatile int	*countp;
  int 		ntasks, pipid;
  int 		niters = 0, i;

  set_sigsegv_watcher();

  if( argc > 1 ) {
    niters = strtol( argv[1], NULL, 10 );
  }
  niters = ( niters == 0 ) ? NITERS : niters;

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ),		       RV, return(EXIT_FAIL) );
  if( pipid == 0 ) {
    mutexp = &mutex;
    CHECK( pip_mutex_init( mutexp ), 		    	       RV, return(EXIT_FAIL) );
    CHECK( pip_named_export(       (void*) mutexp, "MUTEX" ),  RV, return(EXIT_FAIL) );

    barrp = &barr;
    CHECK( pip_barrier_init( barrp, ntasks ), 	    	       RV, return(EXIT_FAIL) );
    CHECK( pip_named_export(       (void*) barrp, "BARRIER" ), RV, return(EXIT_FAIL) );

    count = 0;
    countp = &count;
    CHECK( pip_named_export(       (void*) countp, "COUNT" ),  RV, return(EXIT_FAIL) );
  } else {
    mutexp = NULL;
    CHECK( pip_named_import( 0, (void**) &mutexp, "MUTEX" ),   RV, return(EXIT_FAIL) );
    CHECK( mutexp==NULL,				       RV, return(EXIT_FAIL) );

    barrp = NULL;
    CHECK( pip_named_import( 0, (void**) &barrp, "BARRIER" ),  RV, return(EXIT_FAIL) );
    CHECK( barrp==NULL,				               RV, return(EXIT_FAIL) );

    countp = NULL;
    CHECK( pip_named_import( 0, (void**) &countp, "COUNT" ),   RV, return(EXIT_FAIL) );
    CHECK( countp==NULL,			               RV, return(EXIT_FAIL) );
  }
  CHECK( pip_barrier_wait( barrp ),    RV, return(EXIT_FAIL) );
  DBGF( "countp:%p  mutexp:%p", countp, mutexp );
  for( i=0; i<niters; i++ ) {
    CHECK( pip_mutex_lock( mutexp ),   RV, return(EXIT_FAIL) );
    DBGF( "countp(%p):%d", countp, *countp );
    (*countp) ++;
    CHECK( pip_mutex_unlock( mutexp ), RV, return(EXIT_FAIL) );
  }
  CHECK( pip_barrier_wait( barrp ),    RV, return(EXIT_FAIL) );
  if( pipid == 0 ) {
    DBGF( "count:%d", count );
    CHECK( count!=ntasks*niters,       RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(),                    RV, return(EXIT_FAIL) );
  return 0;
}
