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

//#define DEBUG

#include <libgen.h>
#include <test.h>

int main( int argc, char **argv ) {
  char *dir;
  char *nargv[2] = { NULL, NULL };
  int status, extval;

  set_sigsegv_watcher();
  dir = dirname( argv[0] );
  chdir( dir );

  CHECK( pip_init( NULL, NULL, NULL, 0 ), RV, return(EXIT_FAIL) );

  nargv[0] = "./prog-noexec";
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	 RV!=EACCES,
	 return(EXIT_FAIL) );

  nargv[0] = "./prog-nopie";
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	 RV!=ELIBEXEC,
	 return(EXIT_FAIL) );

  nargv[0] = "./prog-nordynamic";
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	 (RV!=ENOEXEC && RV!=0), /* this MAY depend ... */
	 return(EXIT_FAIL) );

  nargv[0] = "prog-pie";	/* not a path (no slash) */
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		       NULL, NULL, NULL ),
	 (RV!=ESRCH && RV!=0), 	/* this MAY depend ... */
	 return(EXIT_FAIL) );

  nargv[0] = "./prog-pie";	/* correct one */
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		       NULL, NULL, NULL ),
	 RV,
	 return(EXIT_FAIL) );

  CHECK( pip_wait_any( NULL, &status ), RV, return(EXIT_UNTESTED) );

  if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
  } else {
    CHECK( 1, RV, RV=0 );
    extval = EXIT_UNRESOLVED;
  }

  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
