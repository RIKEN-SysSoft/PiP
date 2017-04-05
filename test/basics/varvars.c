/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>

void set_and_print( char *name, int *intp ) {
  int pipid;

  pip_get_pipid( &pipid );
  *intp = pipid;
  fprintf( stderr, "%20s: %p\n", name, intp );
  if( *intp != pipid ) {
    fprintf( stderr, "??????\n" );
  }
}

#define SET_AND_PRINT( VAR ) set_and_print( #VAR, &VAR );

#define CONST_VAL	(12345)

void print_only( char *name, const int *intp ) {
  fprintf( stderr, "%20s: %p\n", name, intp );
  if( *intp != CONST_VAL ) {
    fprintf( stderr, "??????\n" );
  }
}

#define PRINT_ONLY( VAR ) print_only( #VAR, &VAR );

int var_static_global;
__thread int var_tls_global;
int init_static_global = CONST_VAL;
__thread int init_tls_global = CONST_VAL;
const int const_static_global = CONST_VAL;
const __thread int const_tls_global = CONST_VAL;

void print_vars( int pipid ) {
  static int var_static_local;
  static __thread int var_static_tls;
  const static int const_static_local = CONST_VAL;
  const static __thread int const_static_tls = CONST_VAL;
  int var_stack;

  SET_AND_PRINT( var_static_global );
  SET_AND_PRINT( var_tls_global );
  SET_AND_PRINT( var_static_local );
  SET_AND_PRINT( var_static_tls );
  SET_AND_PRINT( var_stack );
  PRINT_ONLY( init_static_global );
  PRINT_ONLY( init_tls_global );
  PRINT_ONLY( const_static_local );
  PRINT_ONLY( const_static_tls );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  int i, err;

  ntasks = NTASKS;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );

  print_vars( pipid );

  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%4, &pipid, NULL, NULL, NULL );
      if( err ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    pip_print_loaded_solibs( stderr );
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }

  } else {
    fprintf( stderr, "<%d> Hello, I am just fine !!\n", pipid );
    pip_print_loaded_solibs( stderr );
  }
  TESTINT( pip_fin() );
  return 0;
}
