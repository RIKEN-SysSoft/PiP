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

#include <test.h>

int main( int argc, char **argv ) {
  int mode, ntasks, nt, pipid, id;
  const char *mode_str = NULL;

  /*** before calling pip_init(), this must fail ***/
  TESTTRUTH( pip_is_initialized(), 	FALSE, 	return(EXIT_FAIL) );
  TESTTRUTH( pip_isa_root(), 		FALSE, 	return(EXIT_FAIL) );
  TESTTRUTH( pip_isa_task(),		FALSE, 	return(EXIT_FAIL) );
  TESTIVAL(  pip_get_pipid( NULL ),	EPERM, 	return(EXIT_FAIL) );
  TESTIVAL(  pip_get_pipid( &pipid ),	EPERM, 	return(EXIT_FAIL) );
  TESTIVAL(  pip_get_mode( NULL ),	EPERM, 	return(EXIT_FAIL) );
  TESTIVAL(  pip_get_mode( &mode ),	EPERM, 	return(EXIT_FAIL) );
  TESTIVAL(  pip_get_ntasks( &ntasks ),	EPERM,	return(EXIT_FAIL) );

  mode_str = pip_get_mode_str();
  if( mode_str != NULL ) {
      fprintf( stderr, "pip_get_mode_str: %s", mode_str );
      return EXIT_FAIL;
  }

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ), return(EXIT_FAIL) );

  /*** after calling pip_init(), this must succeed ***/
  TESTTRUTH( pip_is_initialized(), 	TRUE, 	return(EXIT_FAIL) );
  TESTINT(   pip_get_mode( &mode ),		return(EXIT_FAIL) );
  TESTINT(   pip_get_ntasks( &nt ),		return(EXIT_FAIL) );

  mode_str = pip_get_mode_str();
  TESTTRUTH( mode_str==NULL, 		FALSE,	return(EXIT_FAIL) );
  if( strcmp( mode_str, PIP_ENV_MODE_PTHREAD          ) == 0 ||
      strcmp( mode_str, PIP_ENV_MODE_PROCESS          ) == 0 ||
      strcmp( mode_str, PIP_ENV_MODE_PROCESS_PRELOAD  ) == 0 ||
      strcmp( mode_str, PIP_ENV_MODE_PROCESS_PIPCLONE ) == 0 ) {
    TESTTRUTH( TRUE, TRUE, return(EXIT_FAIL) );
  }

  /* pip_isa_root() */
  if( pip_isa_root() ) {
    TESTINT(   pip_get_pipid( NULL ), 		return(EXIT_FAIL) );
    TESTINT(   pip_get_pipid( &id ), 		return(EXIT_FAIL) );
    TESTTRUTH( id!=pipid, 		FALSE,	return(EXIT_FAIL) );
    TESTTRUTH( id!=PIP_PIPID_ROOT, 	FALSE,	return(EXIT_FAIL) );

  } else if( pip_isa_task() ) {
    char *env;
    int nte;

    TESTINT(   pip_get_pipid( NULL ), 		return(EXIT_FAIL) );
    TESTINT(   pip_get_pipid( &id ), 		return(EXIT_FAIL) );
    TESTTRUTH( id!=pipid, 		FALSE,	return(EXIT_FAIL) );

    env = getenv( PIP_TASK_NUM_ENV );
    TESTTRUTH( env==NULL, 		FALSE,	return(EXIT_FAIL) );

    nte = strtol( env, NULL, 10 );
    TESTTRUTH( nt==nte, 		TRUE,	return(EXIT_FAIL) );

  } else {
    fprintf( stderr, "pip_isa_root and pip_isa_task both fail\n" );
    return EXIT_FAIL;
  }
  TESTINT( pip_fin(), return(EXIT_FAIL) );
  return EXIT_PASS;
}
