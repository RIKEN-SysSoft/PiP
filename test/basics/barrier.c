/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
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
  int i;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }

  TESTINT( pthread_mutex_init( &tc.mutex, NULL ) );
  TESTINT( pthread_mutex_lock( &tc.mutex ) );

  exp    = (void*) &tc;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {

    for( i=0; i<ntasks; i++ ) {
      int err;
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%4, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
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
