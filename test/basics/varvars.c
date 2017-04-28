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

int error = 0;
char tag[64];

void set_and_print( char *name, int *intp, int val ) {
  *intp = val;
  printf( "%20s %32s: %p\n", tag, name, intp );
}
#define SET_AND_PRINT( VAR, VAL ) set_and_print( #VAR, &VAR, VAL );

void check_var( char *name, int *intp, int val ) {
  if( *intp != val ) {
    printf( "%20s %32s: %d !!!!=== %d !!!!\n", tag, name, *intp, val );
    error = 1;
  }
}
#define CHECK_VAR( VAR, VAL ) check_var( #VAR, &VAR, VAL );

#define CONST_VAL	(12345)

void print_only( char *name, const int *intp ) {
  printf( "%20s %32s: %p\n", tag, name, intp );
  if( *intp != CONST_VAL ) {
    printf( "%20s ??????\n", tag );
    error = 1;
  }
}
#define PRINT_ONLY( VAR ) print_only( #VAR, &VAR );

int var_static_global;
__thread int var_tls_global;
int init_static_global              = CONST_VAL;
__thread int init_tls_global        = CONST_VAL;
const int const_static_global       = CONST_VAL;
const __thread int const_tls_global = CONST_VAL;

void print_vars( int pipid ) {
  static int var_static_local;
  static __thread int var_static_tls;
  const static int const_static_local = CONST_VAL;
  const static __thread int const_static_tls = CONST_VAL;
  int var_stack;

  SET_AND_PRINT( var_static_global, pipid );
  SET_AND_PRINT( var_tls_global, pipid );
  SET_AND_PRINT( var_static_local, pipid );
  SET_AND_PRINT( var_static_tls, pipid );
  SET_AND_PRINT( var_stack, pipid );
  PRINT_ONLY( init_static_global );
  PRINT_ONLY( init_tls_global );
  PRINT_ONLY( const_static_local );
  PRINT_ONLY( const_static_tls );
  CHECK_VAR( var_static_local, pipid );
  CHECK_VAR( var_static_tls, pipid );
  CHECK_VAR( var_stack, pipid );
  CHECK_VAR( var_static_global, pipid );
  CHECK_VAR( var_tls_global, pipid );
  fflush( NULL );
}

void check_vars( int pipid ) {
  CHECK_VAR( var_static_global, pipid );
  CHECK_VAR( var_tls_global, pipid );
  fflush( NULL );
}

pthread_barrier_t	barrier;

int main( int argc, char **argv ) {
  pthread_barrier_t *barrp;
  int pipid = 999;
  int ntasks;
  int i, err;

  if( !pip_isa_piptask() ) {
    if( argc < 2 ) {
      ntasks = NTASKS;
    } else {
      ntasks = atoi( argv[1] );
    }
    TESTINT( pthread_barrier_init( &barrier, NULL, ntasks+1 ) );
    barrp = &barrier;
  }
  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  pip_idstr( tag, 64 );

  if( pipid == PIP_PIPID_ROOT ) {
    print_vars( pipid );
    fflush( NULL );

    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%4, &pipid, NULL, NULL, NULL );
      if( err ) break;
      if( i != pipid ) {
	printf( "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    if( argc > 1 ) {
      for( i=0; i<ntasks; i++ ) pthread_barrier_wait( barrp );
    } else {
      pthread_barrier_wait( barrp );
    }
    //pip_print_loaded_solibs( stderr );
    pthread_barrier_wait( barrp );
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }

  } else {

    if( argc > 1 ) {
      for( i=0; i<pipid; i++ ) pthread_barrier_wait( barrp );
    } else {
      pthread_barrier_wait( barrp );
    }
    print_vars( pipid );
    //pip_print_loaded_solibs( stderr );
    if( argc > 1 ) {
      for( i=pipid; i<ntasks; i++ ) pthread_barrier_wait( barrp );
    }
    check_vars( pipid );
    pthread_barrier_wait( barrp );
    if( !error ) {
      printf( "%s Hello, I am just fine !!\n", tag );
    } else {
      printf( "%s Hello, I am NOT fine !!\n", tag );
    }
    fflush( NULL );
  }
  TESTINT( pip_fin() );
  return 0;
}
