/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
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

struct task_comm {
  pthread_mutex_t	mutex;
  pthread_barrier_t	barrier;
};

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  pthread_barrier_t 	*barrp;
  void 			*exp;
  int 	pipid = 999;
  int 	ntasks;
  int 	i, err;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }

  if( !pip_isa_piptask() ) {
    TESTINT( pthread_mutex_init( &tc.mutex, NULL ) );
    TESTINT( pthread_mutex_lock( &tc.mutex ) );
  }

  exp  = (void*) &tc;
  barrp = &tc.barrier;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  pip_idstr( tag, 64 );

  if( pipid == PIP_PIPID_ROOT ) {
    print_vars( pipid );
    fflush( NULL );

    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	printf( "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );
    TESTINT( pthread_mutex_unlock( &tc.mutex ) );

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
    struct task_comm 	*tcp;

    tcp   = (struct task_comm*) exp;
    barrp = (pthread_barrier_t*) &tcp->barrier;
    TESTINT( pthread_mutex_lock( &tcp->mutex ) );
    TESTINT( pthread_mutex_unlock( &tcp->mutex ) );

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
