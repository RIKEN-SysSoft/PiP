/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 1.2.0$
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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
 */

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>

int error = 0;
char tag[64];

#define CONST_VAL	(12345)

int var_static_global;
int init_static_global              = CONST_VAL;
const int const_static_global       = CONST_VAL;

void *var_pointers[10];
static int  nvars;

void set_addrs( void ) {
  int i = 0;

  var_pointers[i++] = (void*) &var_static_global;
  var_pointers[i++] = (void*) &init_static_global;
  var_pointers[i++] = (void*) &const_static_global;
  nvars = i;
}

void check_vars( int pipid ) {
  char *names[] = {
    "var_static_global",
    "init_static_global",
    "const_static_global",
  };
  void **imp, *varp;
  int i;

  TESTINT( pip_get_addr( pipid, "var_pointers", (void**) &imp ) );
  DBGF( "imp@%p", imp );
  for( i=0; i<nvars; i++ ) {
    TESTINT( pip_get_addr( pipid, names[i], &varp ) );
    DBGF( "[%d] %s@%p", pipid, names[i], varp );
    if( imp[i] != varp ) {
      printf( "%20s %32s: %p !!!!=== %p !!!!\n", tag, names[i], imp[i], varp );
      error = 1;
    }
  }
}

struct task_comm {
  pthread_mutex_t	mutex;
  pthread_barrier_t	barrier;
};

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  pthread_barrier_t 	*barrp;
  void 			*exp;
  int pipid = 999;
  int ntasks;
  int i, err;

  if( argc < 2 ) {
    ntasks = NTASKS;
  } else {
    ntasks = atoi( argv[1] );
  }

  if( !pip_isa_piptask() ) {
    TESTINT( pthread_mutex_init( &tc.mutex, NULL ) );
    TESTINT( pthread_mutex_lock( &tc.mutex ) );
  }
  exp  = (void*) &tc;
  barrp = &tc.barrier;

  set_addrs();
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  pip_idstr( tag, 64 );

  if( pipid == PIP_PIPID_ROOT ) {
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
