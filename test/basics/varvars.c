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

int var_static_global;
__thread int var_tls_global;

void print_vars( int pipid ) {
  static int var_static_local;
  static __thread int var_static_tls;
  int var_stack;

  SET_AND_PRINT( var_static_global );
  SET_AND_PRINT( var_tls_global );
  SET_AND_PRINT( var_static_local );
  SET_AND_PRINT( var_static_tls );
  SET_AND_PRINT( var_stack );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;

  ntasks = 1;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );

  print_vars( pipid );

  if( pipid == PIP_PIPID_ROOT ) {
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
    TESTINT( pip_print_loaded_solibs( stderr ) );
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    fprintf( stderr, "<%d> Hello, I am just fine !!\n", pipid );
    TESTINT( pip_print_loaded_solibs( stderr ) );
  }
  return 0;
}
