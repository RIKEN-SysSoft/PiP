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

//#define DEBUG

#include <test.h>
#include <pip_ulp.h>

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
#else
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(250)	/* must be less tahn 256 (0xff) */
# endif
#endif

#define NULPS		(NTASKS-10)
//#define NULPS	(10)
//#define NULPS	(3)

int main( int argc, char **argv ) {
  int ntasks, nulps;
  int i, pipid;

  if( argc   > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  }
  if( ntasks > NTASKS ) {
    fprintf( stderr,
	     "Too many number of PiP tasks (must be less than 256)\n" );
    exit( 1 );
  }
  nulps = ntasks - 1;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_t ulps;

    PIP_ULP_INIT( &ulps );
    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i + 1;
      TESTINT( pip_ulp_new( &prog, &pipid, &ulps, NULL ) );
    }
    pipid = 0;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, &ulps ));
    for( i=0; i<ntasks; i++ ) {
      int status;
      TESTINT( pip_wait( i, &status ) );
      if( status == ( i & 0xff ) ) {
	fprintf( stderr, "Succeeded (%d)\n", i );
      } else {
	fprintf( stderr, "pip_wait(%d!=%d)\n", i & 0xff, status );
      }
    }
  } else {
    fprintf( stderr, "<%d> Hello !!\n", pipid );
    TESTINT( pip_exit( pipid ) );
  }
  TESTINT( pip_fin() );
  return 0;
}
