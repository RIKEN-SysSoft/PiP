/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <eval.h>
#include <pip_machdep.h>

#if defined(PIP)
#include <pip.h>
#endif

#if defined(FORK_ONLY)
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#endif

#define NTASKS_MAX	(16)
#define MMAP_ITER	(1000*1000)
#define MMAP_SIZE	((size_t)(1024*1024))

#define EVAL_MMAP	eval_mmap()
/*
#define EVAL_MMAP	ac_eval_mmap()
*/

double time_start, time_end;

struct sync_st {
  volatile int	sync0;
  volatile int	sync1;
};

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
}

void wait_sync0( struct sync_st *syncs ) {
  __sync_fetch_and_sub( &syncs->sync0, 1 );
  while( syncs->sync0 > 0 ) {
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
  }
}

void wait_sync1( struct sync_st *syncs ) {
  __sync_fetch_and_sub( &syncs->sync1, 1 );
  while( syncs->sync1 > 0 ) {
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
  }
}

void eval_mmap( void ) {
  int i;
  for( i=0; i<MMAP_ITER; i++ ) {
    void *mmapp;
    mmapp = mmap( NULL,
		  MMAP_SIZE,
		  PROT_READ|PROT_WRITE,
		  MAP_PRIVATE|MAP_ANONYMOUS,
		  -1,
		  0 );
    TESTSC( ( mmapp == MAP_FAILED ) );
    TESTSC( munmap( mmapp, MMAP_SIZE ) );
  }
}

#define AC_MMAP_ITER	(100*1000)
#define AC_MMAP_SIZE	((size_t)(4*1024))

void ac_eval_mmap( void ) {
  void *mmap_array[AC_MMAP_ITER];
  int i;
  for( i=0; i<AC_MMAP_ITER; i++ ) {
    mmap_array[i] = mmap( NULL,
			  AC_MMAP_SIZE,
			  PROT_READ|PROT_WRITE,
			  MAP_PRIVATE|MAP_ANONYMOUS,
			  -1,
			  0 );
    TESTSC( ( mmap_array[i] == MAP_FAILED ) );
  }
  for( i=0; i<AC_MMAP_ITER; i++ ) {
    TESTSC( munmap( mmap_array[i], AC_MMAP_SIZE ) );
  }
}

#ifdef PIP
struct sync_st syncs;

void spawn_tasks( char **argv, int ntasks ) {
  void *root_exp;
  int pipid, i;

  init_syncs( ntasks+1, &syncs );
  root_exp = (void*) &syncs;
  TESTINT( pip_init( &pipid, &ntasks, &root_exp, PIP_MODEL_PROCESS ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );
  argv[1] = "0";
  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_spawn( argv[0], argv, NULL, i+1, &pipid, NULL, NULL, NULL ) );
  }
  wait_sync0( &syncs );
  time_start = get_time();
  /* ... */
  wait_sync1( &syncs );
  time_end = get_time();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pip_wait( i, NULL ) );
  }
  TESTINT( pip_fin() );
}

void eval_pip( void ) {
  struct sync_st *syncp;
  void *root_exp;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, &root_exp, 0 ) );
  TESTINT( ( pipid == PIP_PIPID_ROOT ) );
  syncp = (struct sync_st*) root_exp;

  wait_sync0( syncp );
  EVAL_MMAP;
  wait_sync1( syncp );
  TESTINT( pip_fin() );
}
#endif

#ifdef THREAD
struct sync_st syncs;

void eval_thread( void ) {
  wait_sync0( &syncs );
  EVAL_MMAP;
  wait_sync1( &syncs );
  sleep( 5 );
  pthread_exit( NULL );
}

void create_threads( int ntasks ) {
  pthread_t threads[NTASKS_MAX];
  pthread_attr_t attr;
  cpu_set_t cpuset;
  int i;

  init_syncs( ntasks+1, &syncs );
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_attr_init( &attr ) );
    CPU_ZERO( &cpuset );
    CPU_SET( i+1, &cpuset );
    TESTINT( pthread_attr_setaffinity_np( &attr, sizeof(cpuset), &cpuset ) );
    TESTINT( pthread_create( &threads[i],
			     &attr,
			     (void*(*)(void*))eval_thread,
			     NULL ) );
  }
  wait_sync0( &syncs );
  time_start = get_time();
  /* ... */
  wait_sync1( &syncs );
  time_end = get_time();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_join( threads[i], NULL ) );
  }
}
#endif

#ifdef FORK_ONLY

void fork_only( int ntasks ) {
  void *mmapp;
  struct sync_st *syncs;
  cpu_set_t cpuset;
  pid_t pid;
  int i;

  mmapp = mmap( NULL,
		4096,
		PROT_READ|PROT_WRITE,
		MAP_SHARED|MAP_ANONYMOUS,
		-1,
		0 );
  TESTSC( ( mmapp == MAP_FAILED ) );

  syncs = (struct sync_st*) mmapp;
  init_syncs( ntasks+1, syncs );

  for( i=0; i<ntasks; i++ ) {
    CPU_ZERO( &cpuset );
    CPU_SET( i+1, &cpuset );
    TESTSC( sched_setaffinity( getpid(), sizeof(cpuset), &cpuset ) );
    if( ( pid = fork() ) == 0 ) {
      wait_sync0( syncs );
      EVAL_MMAP;
      wait_sync1( syncs );
      exit( 0 );
    }
  }
  wait_sync0( syncs );
  time_start = get_time();
  /* ... */
  wait_sync1( syncs );
  time_end = get_time();
  for( i=0; i<ntasks; i++ ) wait( NULL );
}
#endif

int main( int argc, char **argv ) {
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "Number of tasks must be specified.\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );
  if( ntasks > 0 ) {		/* root process/thread */
    if( ntasks > 16 ) {
      fprintf( stderr, "Illegal number of tasks is specified.\n" );
      exit( 1 );
    }
#if defined( PIP )
    spawn_tasks( argv, ntasks );
#elif defined( THREAD )
    create_threads( ntasks );
#elif defined( FORK_ONLY )
    fork_only( ntasks );
#endif

    printf( "%g [sec], %d tasks, %d iteration, %d [KB] mmap size\n",
	    time_end - time_start, ntasks, MMAP_ITER, ((int)MMAP_SIZE)/1024 );

  } else {			/* child task/process/thread */
#if defined(PIP)
    eval_pip();
#endif
  }
  return 0;
}
