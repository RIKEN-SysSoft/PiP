/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.1$
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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
 */

#define PIP_INTERNAL_FUNCS

//#define DEBUG
#include <test.h>

int error = 0;
char tag[64];

int main( int, char** );

void check_gdbinfo( void ) {
  void *handle = NULL;
  void *mainf  = NULL;
  char *env = getenv( PIP_GDBINFO_ENV );

  if( env == NULL ) {
    error = 1;
  } else if( ( handle = (void*) strtol( env, NULL, 16 ) ) == NULL ) {
    DBGF( "env=%s", env );
    error = 1;
  } else if( ( mainf = dlsym( handle, "main" ) ) == NULL ||
	     mainf != main ) {
    DBGF( "handle=%p  dlsym(main)=%p, main=%p\n", handle, mainf, main );
    error = 1;
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
    check_gdbinfo();
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
