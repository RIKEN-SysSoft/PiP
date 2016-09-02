/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

#include <test.h>

struct comm_st {
  pthread_barrier_t barrier;
  int	go;
};

struct comm_st	comm;

int assign_core( void ) {
  static int init = 0;
  static int core = 0;
  static cpu_set_t allset;
  cpu_set_t cpuset;
  int nc, i;

  if( !init ) {
    init = 1;
    CPU_ZERO( &allset );
    for( i=0; i<sizeof(allset)*8; i++ ) {
      CPU_ZERO( &cpuset );
      CPU_SET( i, &cpuset );
      if( sched_setaffinity( 0, sizeof(allset), &cpuset ) == 0 ) {
	CPU_OR( &allset, &cpuset, &allset );
      }
    }
    if( ( nc = CPU_COUNT( &allset ) ) == 0 ) {
      fprintf( stderr, "No core found\n" );
      return -1;
    } else {
      fprintf( stderr, "%d cores found\n", nc );
    }
  }
  while( 1 ) {
    if( CPU_ISSET( core, &allset ) ) break;
    core++;
    if( core >= sizeof(allset)*8 ) core = 0;
  }
  return core++;
}

int main( int argc, char **argv ) {
  struct comm_st *commp;
  void *exp;
  char corestr[32];
  char *nargv[3];
  int pipid;
  int ntasks;
  int core, i;
  int err;

  ntasks = NTASKS;
  exp    = &comm;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      if( ( core = assign_core() ) < 0 ) break;
      sprintf( corestr, "%d", core );
      nargv[0] = argv[0];
      nargv[1] = corestr;
      nargv[2] = NULL;
      pipid = i;

      fprintf( stderr, "Spawning task[%d] on %s\n", i, corestr );

      err = pip_spawn( nargv[0], nargv, NULL, core, &pipid, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    TESTINT( pthread_barrier_init( &comm.barrier, NULL, ntasks+1 ) );
    comm.go = 1;
    pthread_barrier_wait( &comm.barrier );
    print_numa();
    fflush( NULL );
    comm.go = 0;
    pthread_barrier_wait( &comm.barrier );

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    commp = (struct comm_st*) exp;
    TESTINT( pip_export( &comm ) );
    while( 1 ) {
      if( commp->go ) break;
      pause_and_yield( 10 );
    }
    pthread_barrier_wait( &commp->barrier );
    pthread_barrier_wait( &commp->barrier );
  }
  return 0;
}
