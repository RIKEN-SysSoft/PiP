/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <pthread.h>

//#define DEBUG
#include <test.h>

#define NTIMES		(100)

struct task_comm {
  pthread_mutex_t	mutex[2];
  volatile int		counter;
};

int main( int argc, char **argv ) {
  struct task_comm	tc;
  void 	*exp;
  int pipid = 999;
  int ntasks;
  int i;

  ntasks = 1;
  exp = (void*) &tc;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    TESTINT( pthread_mutex_init( &tc.mutex[0], NULL ) );
    TESTINT( pthread_mutex_init( &tc.mutex[1], NULL ) );
    TESTINT( pthread_mutex_lock( &tc.mutex[0] ) );
    TESTINT( pthread_mutex_lock( &tc.mutex[1] ) );
    tc.counter = -1;

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL) );

    for( i=0; i<NTIMES; i++ ) {
      TESTINT( pthread_mutex_lock(   &tc.mutex[0] ) );
      tc.counter = i;
      TESTINT( pthread_mutex_unlock( &tc.mutex[1] ) );
      //fprintf( stderr, "[ROOT] unlocking mutex (%d)\n", i );
    }
    TESTINT( pthread_mutex_unlock( &tc.mutex[0] ) );

    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    struct task_comm 	*tcp;

    TESTINT( pip_import( PIP_PIPID_ROOT, &exp ) );
    tcp = (struct task_comm*) exp;

    for( i=0; i<NTIMES; i++ ) {
      TESTINT( pthread_mutex_unlock( &tcp->mutex[0] ) );
      TESTINT( pthread_mutex_lock(   &tcp->mutex[1] ) );
      fprintf( stderr, "[TASK] Hello, I am here (counter=%d/%d) !!\n",
	       tcp->counter, i );
    }
    TESTINT( pthread_mutex_unlock( &tcp->mutex[1] ) );
  }
  return 0;
}
