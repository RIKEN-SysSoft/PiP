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

//#define DEBUG

#define PIP_INTERNAL_FUNCS
#include <test.h>

pip_clone_t *pip_get_cloneinfo_( void );

void print_task_info( int pipid ) {
  pip_clone_t *info = pip_get_cloneinfo_();
  int stack;
  char tag[64];

  pip_idstr( tag, 64 );
  pip_check_addr( tag, &stack );
#ifdef __x86_64__
  unsigned long fssegr;
  arch_prctl( ARCH_GET_FS, &fssegr );
  pip_check_addr( tag, (void*) fssegr );
  if( info != NULL ) {
    pip_check_addr( tag, info->stack );
    fprintf( stderr,
	     "%s FS=%p  stack=%p  clone_stack=%p  pthread_self()=%p\n",
	     tag, (void*) fssegr, &stack, info->stack,
	     (void*) pthread_self() );
    fprintf( stderr,
	     "%s FS*=0x%lx  STACK*=0x%lx  pthread_self()*=0x%lx\n",
	     tag,
	     ((intptr_t)fssegr) - ((intptr_t)info->stack),
	     ((intptr_t)&stack) - ((intptr_t)info->stack),
	     ((intptr_t)((void*)pthread_self())) - ((intptr_t)info->stack) );
  } else {
    fprintf( stderr,
	     "%s FS=%p  stack=%p  pthread_self()=%p\n",
	     tag, (void*) fssegr, &stack, (void*) pthread_self() );
  }
#else
  if( info != NULL ) {
    fprintf( stderr,
	     "%s  stack=%p  clone_stack=%p  pthread_self()=%p\n",
	     tag, &stack, info->stack, (void*) pthread_self() );
  } else {
    fprintf( stderr,
	     "%s  stack=%p  pthread_self()=%p\n",
	     tag, &stack, (void*) pthread_self() );
  }
#endif
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
  int i;
  int err;

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
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
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

    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );
    TESTINT( pthread_mutex_unlock( &tc.mutex ) );

    pthread_barrier_wait( barrp );
    print_maps();
    pthread_barrier_wait( barrp );

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    struct task_comm 	*tcp;

    tcp   = (struct task_comm*) exp;
    barrp = (pthread_barrier_t*) &tcp->barrier;
    TESTINT( pthread_mutex_lock( &tcp->mutex ) );
    TESTINT( pthread_mutex_unlock( &tcp->mutex ) );

    print_task_info( pipid );

    fflush( NULL );
    pthread_barrier_wait( barrp );
    pthread_barrier_wait( barrp );
  }
  return 0;
}
