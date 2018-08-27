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
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <semaphore.h>

#define PIP_INTERNAL_FUNCS
//#define DEBUG
#include <test.h>

char tag[64];

void set_and_print( char *name, int *intp, int val ) {
  *intp = val;
  fprintf( stderr, "%20s %32s: %p\n", tag, name, intp );
}
#define SET_AND_PRINT( VAR, VAL ) set_and_print( #VAR, &VAR, VAL );

int check_var( char *name, int *intp, int val ) {
  if( *intp != val ) {
    fprintf( stderr, "%20s %32s: %d !!!!=== %d !!!!\n", tag, name, *intp, val);
    return 0;
  }
  return 1;
}
#define CHECK_VAR( VAR, VAL ) check_var( #VAR, &VAR, VAL )

#define CONST_VAL	(12345)

int print_only( char *name, const int *intp ) {
  fprintf( stderr, "%20s %32s: %p\n", tag, name, intp );
  if( *intp != CONST_VAL ) {
    fprintf( stderr, "%20s ??????\n", tag );
    return 0;
  }
  return 1;
}
#define PRINT_ONLY( VAR ) print_only( #VAR, &VAR )

int var_static_global;
__thread int var_tls_global;
int init_static_global              = CONST_VAL;
__thread int init_tls_global        = CONST_VAL;
const int const_static_global       = CONST_VAL;
const __thread int const_tls_global = CONST_VAL;

int print_vars( int pipid ) {
  static int var_static_local;
  static __thread int var_static_tls;
  const static int const_static_local = CONST_VAL;
  const static __thread int const_static_tls = CONST_VAL;
  int var_stack;

  SET_AND_PRINT( var_static_global, pipid );
  SET_AND_PRINT( var_tls_global,    pipid );
  SET_AND_PRINT( var_static_local,  pipid );
  SET_AND_PRINT( var_static_tls,    pipid );
  SET_AND_PRINT( var_stack,         pipid );
  if( PRINT_ONLY( init_static_global )      ||
      PRINT_ONLY( init_tls_global )         ||
      PRINT_ONLY( const_static_local )      ||
      PRINT_ONLY( const_static_tls )        ||
      CHECK_VAR( var_static_local,  pipid ) ||
      CHECK_VAR( var_static_tls,    pipid ) ||
      CHECK_VAR( var_stack,         pipid ) ||
      CHECK_VAR( var_static_global, pipid ) ||
      CHECK_VAR( var_tls_global, pipid ) ) {
    fflush( NULL );
    return 0;
  }
  fflush( NULL );
  return 1;
}

int check_vars( int pipid ) {
  if( CHECK_VAR( var_static_global, pipid ) ||
      CHECK_VAR( var_tls_global,    pipid ) ) {
    fflush( NULL );
    return 0;
  }
  fflush( NULL );
  return 1;
}

struct expo {
  sem_t		     semaphore;
  pip_ulp_barrier_t  barr;
  pip_locked_queue_t queue;
} expo;

void wakeup_task( pip_task_t *task ) {
  struct expo *expop;

  TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
  TESTINT( sem_post( &expop->semaphore ) );
}

int main( int argc, char **argv ) {
  struct expo	*expop = &expo;
  int 	pipid = 999;
  int 	ntasks, nulps;
  int 	st, ext = 0, i, err;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  nulps = ntasks - 1;

  TESTINT( pip_init( &pipid, &ntasks, (void**) &expop, 0 ) );

  if( pipid == PIP_PIPID_ROOT ) {
    pip_ulp_barrier_init( &expop->barr, ntasks );
    TESTINT( pip_locked_queue_init( &expop->queue ) );

    pip_idstr( tag, 64 );
    (void) print_vars( pipid );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n", i, NTASKS, strerror(err) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, &st ) );
      if( st != 0 ) ext = 1;
    }
    if( !ext ) printf( "OK\n" );

  } else {
    int error;
    if( pipid == ntasks - 1 ) {
      for( i=0; i<nulps; i++ ) {
	TESTINT( sem_wait( &expop->semaphore ) );
	TESTINT( pip_ulp_dequeue_and_involve( &expop->queue, NULL, 0 ) );
      }
    } else {
      /* become ULP */
      TESTINT( pip_sleep_and_enqueue( &expop->queue, wakeup_task, 0 ) );
    }
    pip_idstr( tag, 64 );
    if( !print_vars( pipid ) ) error = 1;
    TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
    if( !check_vars( pipid ) ) error = 1;
    TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
    if( error ) {
      fprintf( stderr, "%s Hello, I am just fine !!\n", tag );
    } else {
      fprintf( stderr, "%s Hello, I am NOT fine !!\n", tag );
      ext = 1;
    }
    fflush( NULL );
  }
  TESTINT( pip_fin() );
  return ext;	       /* this results in scheduling the other ULPs */
}
