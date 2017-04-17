/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG

#include <pthread.h>

#define PIP_INTERNAL_FUNCS
#include <test.h>
#include <pip_internal.h>

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

pthread_barrier_t	barrier;

int main( int argc, char **argv ) {
  pthread_barrier_t *barrp;
  int pipid = 999;
  int ntasks;
  int i;
  int err;

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
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%8, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;

    pthread_barrier_wait( barrp );
    print_maps();
    pthread_barrier_wait( barrp );

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    print_task_info( pipid );
    fflush( NULL );
    pthread_barrier_wait( barrp );
    pthread_barrier_wait( barrp );
  }
  return 0;
}
