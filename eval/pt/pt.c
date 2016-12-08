/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#include <eval.h>
#include <pip_machdep.h>

#if defined(PIP)
#include <pip.h>
#endif

#if defined(FORK_ONLY)
#include <unistd.h>
#include <sched.h>
#endif

#define NTASKS_MAX	(50)

struct sync_st {
  volatile int        sync0;
  volatile int        sync1;
};

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
}

void wait_sync( volatile int *syncp ) {
  __sync_fetch_and_sub( syncp, 1 );
  while( *syncp > 0 ) {
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
  }
}

void sync2( struct sync_st *syncs ) {
  wait_sync( &syncs->sync0 );
  wait_sync( &syncs->sync1 );
}

void print_page_table_size( void ) {
  system( "grep PageTables /proc/meminfo" );
}

void print_pte_size( void ) {
#ifdef AH
  char sysstr[128];
  sprintf( sysstr, "echo -n '[PID=%d]'; grep PTE /proc/%d/status",
	   getpid(), getpid() );
  system( sysstr );
#endif
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
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ) );
  }
  wait_sync( &syncs.sync0 );
  print_page_table_size();
  wait_sync( &syncs.sync1 );

  for( i=0; i<ntasks; i++ ) wait( NULL );

  TESTINT( pip_fin() );
}

void eval_pip( char *argv[] ) {
  struct sync_st *syncp;
  void *root_exp;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, &root_exp, 0 ) );
  TESTINT( ( pipid == PIP_PIPID_ROOT ) );
  syncp = (struct sync_st*) root_exp;

  wait_sync( &syncp->sync0 );
  print_pte_size();
  wait_sync( &syncp->sync1 );
}

#endif

#ifdef THREAD

struct sync_st syncs;

void eval_thread( void ) {
  wait_sync( &syncs.sync0 );
  print_pte_size();
  wait_sync( &syncs.sync1 );

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
    TESTINT( pthread_create( &threads[i],
			     &attr,
			     (void*(*)(void*))eval_thread,
			     NULL ) );
  }
  wait_sync( &syncs.sync0 );
  print_page_table_size();
  wait_sync( &syncs.sync1 );

  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_join( threads[i], NULL ) );
  }
}
#endif

#ifdef FORK_ONLY

void fork_only( int ntasks ) {
  void *mmapp;
  struct sync_st *syncs;
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
    if( ( pid = fork() ) == 0 ) {

      wait_sync( &syncs->sync0 );
      print_pte_size();
      wait_sync( &syncs->sync1 );

      exit( 0 );
    }
  }
  wait_sync( &syncs->sync0 );
  print_page_table_size();
  wait_sync( &syncs->sync1 );

  for( i=0; i<ntasks; i++ ) wait( NULL );
}
#endif

int main( int argc, char **argv ) {
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "pt-XXX <ntasks>\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );

  if( ntasks > 0 ) {		/* root process/thread */
    if( ntasks > NTASKS_MAX ) {
      fprintf( stderr, "Illegal number of tasks is specified.\n" );
      exit( 1 );
    }
    print_page_table_size();

#if defined( PIP )
    spawn_tasks( argv, ntasks );
#elif defined( THREAD )
    create_threads( ntasks );
#elif defined( FORK_ONLY )
    fork_only( ntasks );
#endif
    print_page_table_size();
  } else {			/* child task/process/thread */
#if defined(PIP)
    eval_pip( argv );
#endif
  }
  return 0;
}
