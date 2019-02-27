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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>

#include <pthread.h>

#define NTIMES		(100)

struct task_comm {
  pthread_mutex_t	mutex;
  pthread_barrier_t	barrier;
};

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  void 	*exp;
  int pipid = 999;
  int ntasks;
  int i, err;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }

  if( !pip_isa_piptask() ) {
    TESTINT( pthread_mutex_init( &tc.mutex, NULL ) );
    TESTINT( pthread_mutex_lock( &tc.mutex ) );
  }

  exp = (void*) &tc;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, ntasks, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;

    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );
    TESTINT( pthread_mutex_unlock( &tc.mutex ) );

    for( i=0; i<NTIMES; i++ ) {
      pthread_barrier_wait( &tc.barrier );
      //fprintf( stderr, "[ROOT] barrier[%d]\n", i );
    }
    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    struct task_comm 	*tcp;

    tcp = (struct task_comm*) exp;
    TESTINT( pthread_mutex_lock( &tcp->mutex ) );
    TESTINT( pthread_mutex_unlock( &tcp->mutex ) );

    for( i=0; i<NTIMES; i++ ) {
      pthread_barrier_wait( &tcp->barrier );
      //sleep( 1 );
      //fprintf( stderr, "[%d] barrier[%d]\n", pipid, i );
    }
    fprintf( stderr, "<%d> Hello, I am fine !!\n", pipid );
  }
  return 0;
}
