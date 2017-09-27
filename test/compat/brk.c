/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <unistd.h>
#include <test.h>

void print_brk( int id ) {
  errno = 0;
  printf( "<%d> brk: %p (err=%d)\n", id, sbrk( 4096 ), errno );
}

struct task_comm {
  pthread_mutex_t	mutex;
  pthread_barrier_t	barrier;
};

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  void 			*exp;
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
      if( err ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d)=%d !!!!!!\n", i, pipid, err );
	break;
      }
    }
    ntasks = i;

    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );
    TESTINT( pthread_mutex_unlock( &tc.mutex ) );

    pthread_barrier_wait( &tc.barrier );
    print_brk( pipid );
    pthread_barrier_wait( &tc.barrier );

    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    struct task_comm 	*tcp;

    tcp = (struct task_comm*) exp;
    TESTINT( pthread_mutex_lock( &tcp->mutex ) );
    TESTINT( pthread_mutex_unlock( &tcp->mutex ) );

    pthread_barrier_wait( &tcp->barrier );
    print_brk( pipid );
    pthread_barrier_wait( &tcp->barrier );
  }
  TESTINT( pip_fin() );
  return 0;
}
