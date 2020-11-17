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
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <sys/wait.h>
#include <test.h>

static int check_sid( int argc, char **argv ) {
  pid_t pid, sid_old, sid_new;
  int  	i, ntasks, ntenv, pipid;
  char	*env;

  ntasks = NTASKS;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0 ) ? NTASKS : ntasks;
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  } else {
    if( ntasks > NTASKS ) return(EXIT_UNTESTED);
  }

  pid = getpid();
  CHECK( ( sid_old = getsid(pid) ), RV<0, return(EXIT_FAIL) );
  CHECK( pip_init(&pipid, &ntasks,NULL,PIP_MODE_PROCESS),
	 RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( (sid_new=setsid()), RV<0, return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    for( i=0; i<ntasks; i++ ) {
      CHECK( pip_wait( i, NULL ), RV, return(EXIT_FAIL) );
    }
    CHECK( ( sid_new == getsid( pid) ), RV, return(EXIT_FAIL) );

  } else {	/* PIP child task */
    CHECK( ( sid_new = setsid() ), RV<0, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(),  RV, return(EXIT_FAIL) );
  return 0;
}

int main( int argc, char **argv ) {
  pid_t pid, sid;
  int rv, status;

  pid = getpid();
  CHECK( ( sid = getsid(pid) ), RV<0, return(EXIT_FAIL) );
  if( sid != pid ) {
    printf( "fork\n" );
    rv = check_sid( argc, argv );
  } else if( ( pid = fork() ) == 0 ) {
    exit( check_sid( argc, argv ) );
  } else {
    wait( &status );
    if( WIFEXITED( status ) ) {
      rv = WEXITSTATUS( status );
    } else {
      CHECK( "test program signaled", 1, return(EXIT_UNRESOLVED) );
    }
  }
  return rv;
}
