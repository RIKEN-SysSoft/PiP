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

#include <libgen.h>
#include <test.h>

static int wait_termination( void ) {
  int status, err;

  err = pip_wait_any( NULL, &status );
  if( err ) return -err;
  if( WIFEXITED( status ) ) {
    return WEXITSTATUS( status );
  } else if( WIFSIGNALED( status ) ) {
    return -9999;
  }
  return err;
}

int main( int argc, char **argv ) {
  char *dir;
  char *nargv[2] = { NULL, NULL };
  int err;

  dir = dirname( argv[0] );
  chdir( dir );

  CHECK( pip_init( NULL, NULL, NULL, 0 ), RV, return(EXIT_FAIL) );

  nargv[0] = "./prog-noexec";
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		      NULL, NULL, NULL ),
	 RV!=EACCES,
	 return(EXIT_FAIL) );

  nargv[0] = "./prog-nopie";
  err = pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		   NULL, NULL, NULL );
  /* this MAY succeed depending on system */
  CHECK( RV=err,
	 (RV!=ELIBEXEC && RV!=0),
	 return(EXIT_FAIL) );
  if( !err ) {
    CHECK( wait_termination(), RV, return(EXIT_FAIL) );
  }

  nargv[0] = "./prog-nordynamic";
  err = pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		   NULL, NULL, NULL );
  /* this MAY succeed depending on system */
  CHECK( RV=err,
	 (RV!=ENOEXEC && RV!=0),
	 return(EXIT_FAIL) );
  if( !err ) {
    CHECK( wait_termination(), RV, return(EXIT_FAIL) );
  }

  nargv[0] = "prog-pie";	/* not a path (no slash) */
  err = pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		   NULL, NULL, NULL );
  CHECK( RV=err,
	 (RV!=ENOENT && RV!=0),
	 return(EXIT_FAIL) );
  if( !err ) {
    CHECK( wait_termination(), RV, return(EXIT_FAIL) );
  }

  nargv[0] = "./prog-pie";	/* correct one */
  CHECK( pip_spawn( nargv[0], nargv, NULL, PIP_CPUCORE_ASIS, NULL,
		       NULL, NULL, NULL ),
	 RV,
	 return(EXIT_FAIL) );
  CHECK( wait_termination(), RV, return(EXIT_FAIL) );

  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return EXIT_PASS;
}
