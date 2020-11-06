/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
 * $PIP_VERSION: Version 1.2.0$
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

#define _GNU_SOURCE
#include <test.h>

#define PIPENV	"PIPENV"
#define PIPENV0	"PIPENV-0"
#define PIPENV1	"PIPENV-1"
#define PIPENV2	"PIPENV-2"

int main( int argc, char **argv ) {
  extern char **environ;
  char *envv[4];
  int pipid, ntasks;
  int i, j;
  int err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {

      j  = 0;
      TESTSYSERR( asprintf( &envv[j++], "%s=%d", PIPENV0, i+10 ) );
      TESTSYSERR( asprintf( &envv[j++], "%s=%d", PIPENV1, i+100 ) );
      TESTSYSERR( asprintf( &envv[j++], "%s=%d", PIPENV2, i+1000 ) );
      envv[j] = NULL;

      pipid = i;
      err = pip_spawn( argv[0], argv, envv, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );

      free( envv[0] );
      free( envv[1] );
      free( envv[2] );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }

      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
    DBG;

    TESTINT( pip_fin() );

  } else {
    char *env, *key;
    int val;

    if( ( env = getenv( PIPENV0 ) ) == NULL ) {
      fprintf( stderr, "<%d> Env:%s not found\n", pipid, PIPENV0 );
      DUMP_ENV( environ, 5 );
    } else if( ( val = atoi( env ) ) != ( pipid + 10 ) ) {
      fprintf( stderr, "<%d> %s=%d (!=%d)\n", pipid, PIPENV0, val, pipid+10 );
      DUMP_ENV( environ, 5 );
    } else if( ( env = getenv( PIPENV1 ) ) == NULL ) {
      fprintf( stderr, "<%d> Env:%s not found\n", pipid, PIPENV1 );
      DUMP_ENV( environ, 5 );
    } else if( ( val = atoi( env ) ) != ( pipid + 100 ) ) {
      fprintf( stderr, "<%d> %s=%d (!=%d)\n", pipid, PIPENV1, val, pipid+100 );
      DUMP_ENV( environ, 5 );
    } else if( ( env = getenv( PIPENV2 ) ) == NULL ) {
      fprintf( stderr, "<%d> Env:%s not found\n", pipid, PIPENV2 );
      DUMP_ENV( environ, 5 );
    } else if( ( val = atoi( env ) ) != ( pipid + 1000 ) ) {
      fprintf( stderr, "<%d> %s=%d (!=%d)\n", pipid, PIPENV2, val, pipid+1000 );
      DUMP_ENV( environ, 5 );
    } else {
      int err = 0;

      TESTSYSERR( asprintf( &key, "%s:%d", PIPENV, pipid ) );
      for( i=0; i<1000; i++ ) {
	TESTSYSERR( asprintf( &env, "%s=%d", key, i+pipid ) );
	//fprintf( stderr, "<%d> putenv(%s)\n", pipid, env );
	if( putenv( env ) != 0 ) {
	  err = 1;
	  fprintf( stderr, "<%d> putenv(%s)=%d\n", pipid, env, errno );
	} else if( ( env = getenv( key ) ) == 0 ) {
	  err = 1;
	  fprintf( stderr, "<%d> Env:%s not found\n", pipid, key );
	} else if( ( val = atoi( env ) ) != i+pipid ) {
	  err = 1;
	  fprintf( stderr, "<%d> %s=%d (!=%d)\n", pipid, key, val, i+pipid );
	}
	//fprintf( stderr, "<%d> getenv(%s)=%d\n", pipid, key, val );
      }
      if( !err ) {
	fprintf( stderr, "<%d:%d> Hello, I am very fine !!\n", pipid, getpid() );
      } else {
	fprintf( stderr, "<%d> Hey, I feel very BAD '&$#'&$\n", pipid );
      }
    }
  }
  DBG;
  return 0;
}
