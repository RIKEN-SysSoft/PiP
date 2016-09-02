/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
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
      asprintf( &envv[j++], "%s=%d", PIPENV0, i+10 );
      asprintf( &envv[j++], "%s=%d", PIPENV1, i+100 );
      asprintf( &envv[j++], "%s=%d", PIPENV2, i+1000 );
      envv[j] = NULL;

      pipid = i;
      err = pip_spawn( argv[0], argv, envv, i%4, &pipid, NULL, NULL );

      free( envv[0] );
      free( envv[1] );
      free( envv[2] );
      if( err != 0 ) break;

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

      asprintf( &key, "%s:%d", PIPENV, pipid );
      for( i=0; i<1000; i++ ) {
	asprintf( &env, "%s=%d", key, i+pipid );
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
	fprintf( stderr, "<%d> Hello, I am very fine !!\n", pipid );
      } else {
	fprintf( stderr, "<%d> Hey, I feel very BAD '&$#'&$\n", pipid );
      }
    }
  }
  DBG;
  return 0;
}
