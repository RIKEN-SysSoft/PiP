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
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>

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

#define NTASKS_MAX	(256)
#define SIZE1MB		((size_t)(1*1024*1024))

//#define USE_HUGETLB

void 	*mmapp;
size_t	size;

struct sync_st {
  void 		*mmapp;
  size_t	size;
  volatile int	sync0;
  volatile int	sync1;
  volatile int	sync2;
};

struct sync_st syncs;

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
  syncs->sync2 = n;
}

void wait_sync( volatile int *syncp ) {
  __sync_fetch_and_sub( syncp, 1 );
  while( *syncp > 0 ) {
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
    sched_yield();
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
  }
}

void wait_sync0( struct sync_st *syncs ) {
  wait_sync( &syncs->sync0 );
}

void wait_sync1( struct sync_st *syncs ) {
  wait_sync( &syncs->sync1 );
}

void wait_sync2( struct sync_st *syncs ) {
  wait_sync( &syncs->sync2 );
}

int get_page_table_size( void ) {
  FILE *fp;
  int ptsz = 0;

  sleep( 3 );

#ifdef AH
  system( "grep PageTables /proc/meminfo" );
#ifdef USE_HUGETLB
  system( "grep HugePages  /proc/meminfo" );
#endif
#endif

  fp = popen( "grep PageTables /proc/meminfo", "r" );
  fscanf( fp, "%*s %d", &ptsz );
  //printf( "PTSZ= %d\n", ptsz );
  pclose( fp );
  return ptsz;
}

void touch( struct sync_st *syncs ) {
  void *p = syncs->mmapp;
  void *q = p + syncs->size;
  for( ; p<q; p+=4096 ) {
    int *ip = (int*) p;
    *ip = 123456;
  }
}

void mmap_memory( char *arg, int ntasks ) {
  if( ( size = atoi( arg ) ) == 0 ) {
    size = SIZE1MB;
  } else {
    size *= SIZE1MB;
  }
  //size *= ntasks;
  mmapp = mmap( NULL,
		size,
		PROT_READ|PROT_WRITE,
#ifdef USE_HUGETLB
		MAP_HUGETLB |
#endif
		MAP_SHARED|MAP_ANONYMOUS,
		-1,
		0 );
  TESTSC( ( mmapp == MAP_FAILED ) );
}

#ifdef PIP

int spawn_tasks( char **argv, int ntasks ) {
  void *root_exp;
  int pipid, ptsz, i;

  syncs.mmapp = mmapp;
  syncs.size  = size;
  init_syncs( ntasks+1, &syncs );
  root_exp = (void*) &syncs;
  TESTINT( pip_init( &pipid, &ntasks, &root_exp, 0 ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );
  argv[1] = "0";
  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid, NULL, NULL, NULL ) );
  }

  wait_sync0( &syncs );
  touch( &syncs );
  wait_sync1( &syncs );
  ptsz = get_page_table_size();
  wait_sync2( &syncs );

  for( i=0; i<ntasks; i++ ) pip_wait( i, NULL );

  TESTINT( pip_fin() );
  return ptsz;
}

void eval_pip( char *argv[] ) {
  struct sync_st *syncp;
  void *root_exp;
  int pipid;

  TESTINT( pip_init( &pipid, NULL, &root_exp, 0 ) );
  TESTINT( ( pipid == PIP_PIPID_ROOT ) );
  syncp = (struct sync_st*) root_exp;
  mmapp = syncp->mmapp;

  wait_sync0( syncp );
  touch( syncp );
  wait_sync1( syncp );
  wait_sync2( syncp );

  TESTINT( pip_fin() );
}
#endif

#ifdef THREAD

void eval_thread( void ) {
  wait_sync0( &syncs );
  touch( &syncs );
  wait_sync1( &syncs );
  wait_sync2( &syncs );
  pthread_exit( NULL );
}

int create_threads( int ntasks ) {
  pthread_t threads[NTASKS_MAX];
  int ptsz, i;

  init_syncs( ntasks+1, &syncs );
  syncs.mmapp = mmapp;
  syncs.size  = size;
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_create( &threads[i],
			     NULL,
			     (void*(*)(void*))eval_thread,
			     NULL ) );
  }
  wait_sync0( &syncs );
  touch( &syncs );
  wait_sync1( &syncs );
  ptsz = get_page_table_size();
  wait_sync2( &syncs );

  for( i=0; i<ntasks; i++ ) {
    TESTINT( pthread_join( threads[i], NULL ) );
  }
  return ptsz;
}
#endif

#ifdef FORK_ONLY

int fork_only( int ntasks ) {
  void *mapp;
  struct sync_st *syncp;
  pid_t pid;
  int ptsz, i;

  mapp = mmap( NULL,
	       4096,
	       PROT_READ|PROT_WRITE,
	       MAP_SHARED|MAP_ANONYMOUS,
	       -1,
	       0 );
  TESTSC( ( mapp == MAP_FAILED ) );

  syncp = (struct sync_st*) mapp;
  init_syncs( ntasks+1, syncp );
  syncp->mmapp = mmapp;
  syncp->size  = size;

  for( i=0; i<ntasks; i++ ) {
    if( ( pid = fork() ) == 0 ) {
      wait_sync0( syncp );
      touch( syncp );
      wait_sync1( syncp );
      wait_sync2( syncp );
      exit( 0 );
    }
  }
  wait_sync0( syncp );
  touch( syncp );
  wait_sync1( syncp );

  ptsz = get_page_table_size();
  wait_sync2( syncp );
  for( i=0; i<ntasks; i++ ) wait( NULL );
  return ptsz;
}
#endif

int main( int argc, char **argv ) {
  int ntasks;
  int ptsz0, ptsz1;

  if( argc < 3 ) {
    fprintf( stderr, "mmap-XXX <ntasks> <mmap-size[KB]>\n" );
    exit( 1 );
  }

  ntasks = atoi( argv[1] );

  if( ntasks > 0 ) {		/* root process/thread */
    if( ntasks > NTASKS_MAX ) {
      fprintf( stderr, "Illegal number of tasks is specified.\n" );
      exit( 1 );
    }
    ptsz0 = get_page_table_size();
    mmap_memory( argv[2], ntasks );

#if defined( PIP )
    ptsz1 = spawn_tasks( argv, ntasks );
#elif defined( THREAD )
    ptsz1 = create_threads( ntasks );
#elif defined( FORK_ONLY )
    ptsz1 = fork_only( ntasks );
#endif
#ifdef AH
    printf( "%d, [KB]  %d tasks  %d [MiB] mmap size\n",
	    ptsz1 - ptsz0, ntasks, (int)(size/SIZE1MB) );
#endif
    printf( ", %d, [KB]\n", ptsz1 - ptsz0 );

  } else {			/* child task/process/thread */
#if defined(PIP)
    eval_pip( argv );
#endif
  }
  return 0;
}
