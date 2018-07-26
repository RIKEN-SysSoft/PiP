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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#include <test.h>
#include <pip_ulp.h>

//#define DEBUG

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
# define NULPS		(4)
#else
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(250)
# endif
# define NULPS		(4)
#endif

int my_ulp_main( void ) {
  int pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  return pipid;
}

int my_tsk_main( void ) {
  int pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  return pipid;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog_ulp, prog_tsk;
  pip_ulp_t 		ulps;
  int			ntasks=0, nulps=0, pipid;
  int 			total, i, j, k;

  if( argc == 1 ) {
    nulps  = NULPS;
    ntasks = NTASKS / ( nulps + 1 );
  } else {
    if( argc > 1 ) ntasks = atoi( argv[1] );
    if( argc > 2 ) nulps  = atoi( argv[2] );
  }
  if( ntasks == 0 ) {
      fprintf( stderr, "Not enough number of tasks\n" );
      exit( 1 );
  }
  total = ntasks * nulps + ntasks;
  if( total > NTASKS ) {
    fprintf( stderr, "Too large\n" );
    exit( 1 );
  }

  pip_spawn_from_func( &prog_ulp, argv[0], "my_ulp_main", NULL, NULL );
  pip_spawn_from_func( &prog_tsk, argv[0], "my_tsk_main", NULL, NULL );
  TESTINT( pip_init( NULL, &total, NULL, 0 ) );

  k = 0;
  while( k < ntasks ) {
    PIP_ULP_INIT( &ulps );
    //fprintf( stderr, "i=%d/%d\n", i, ntasks );
    for( j=0; j<nulps; j++ ) {
      if( k >= ntasks - 2 ) break;
      //fprintf( stderr, "i=%d/%d  j=%d/%d\n", i, ntasks, j, NULPS );
      pipid = k++;
      TESTINT( pip_ulp_new( &prog_ulp, &pipid, &ulps, NULL ) );
    }
    pipid = k++;
    TESTINT( pip_task_spawn( &prog_tsk,
			     PIP_CPUCORE_ASIS,
			     &pipid,
			     NULL,
			     &ulps ));
  }
  for( i=0; i<k; i++ ) {
    int status;
    TESTINT( pip_wait_any( &pipid, &status ) );
    if( status == pipid && status == i ) {
      fprintf( stderr, "[%d] Succeeded (%d)\n", pipid, status );
    } else {
      fprintf( stderr, "[%d] pip_wait_any(%d):%d -- FAILED\n",
	       pipid, i, status );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
