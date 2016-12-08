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

#define NTASKS_MAX	(50)
#define MMAP_SIZE	((size_t)(1024*1024))

#ifndef TOUCH

#ifndef ACUM
//#define MMAP_ITER	(10*1000)
#define MMAP_ITER	(1000*1000)
//#define MMAP_ITER	(10*1000*1000)
#else
#define MMAP_ITER	(100*1000)
#endif

#else

#ifndef ACUM
#define MMAP_ITER	(10*1000)
#else
#define MMAP_ITER	(1*1000)
#endif

#endif

//#define MMAP_PROT	(PROT_READ|PROT_WRITE)
#define MMAP_PROT	(PROT_READ)
//#define MMAP_OPTS	(MAP_PRIVATE|MAP_ANONYMOUS|MAP_POPULATE)
#define MMAP_OPTS	(MAP_PRIVATE|MAP_ANONYMOUS)

size_t	size;
double	time_start, time_end;
double	time_start2, time_end2;

struct sync_st {
  volatile int	sync0;
  volatile int	sync1;
  volatile int	sync2;
  volatile int	sync3;
};

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
  syncs->sync2 = n;
  syncs->sync3 = n;
}

void wait_sync( volatile int *syncp ) {
  __sync_fetch_and_sub( syncp, 1 );
  while( *syncp > 0 ) {
    pip_pause(); pip_pause(); pip_pause(); pip_pause(); pip_pause();
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

void wait_sync3( struct sync_st *syncs ) {
  wait_sync( &syncs->sync3 );
}

#define BUFSZ	1024
void print_maps( void ) {
#ifdef ACUM
  int fd = open( "/proc/self/maps", O_RDONLY );
  char buf[BUFSZ];

  while( 1 ) {
    ssize_t rc, wc;
    char *p;

    if( ( rc = read( fd, buf, BUFSZ ) ) <= 0 ) break;
    p = buf;
    do {
      if( ( wc = write( 1, p, rc ) ) < 0 ) break; /* STDOUT */
      p  += wc;
      rc -= wc;
    } while( rc > 0 );
  }
  close( fd );
#endif
}

void touch( void *p ) {
#ifdef TOUCH
  void *q = p + size;
  for( ; p<q; p+=4096 ) {
    int *ip = (int*) p;
    *ip = 123456;
  }
#endif
}

void mmap_size( char *arg ) {
  if( ( size = atoi( arg ) ) == 0 ) {
    size = MMAP_SIZE;
  } else {
    size *= 1024;
  }
}
#ifndef ACUM
void par_eval_mmap( void ) {
  int i;
  for( i=0; i<MMAP_ITER; i++ ) {
    void *mmapp;
    mmapp = mmap( NULL,
		  size,
		  MMAP_PROT,
		  MMAP_OPTS,
		  -1,
		  0 );
    TESTSC( ( mmapp == MAP_FAILED ) );
    TESTSC( ( mmapp == NULL       ) );
    touch( mmapp );
    TESTSC( munmap( mmapp, size ) );
  }
}
#endif

#ifdef ACUM
void *mmap_array[MMAP_ITER];

void ac_eval_mmap( void ) {
  int i;
  for( i=0; i<MMAP_ITER; i++ ) {
    mmap_array[i] = mmap( NULL,
			  size,
			  MMAP_PROT,
			  MMAP_OPTS,
			  -1,
			  0 );
    TESTSC( ( mmap_array[i] == MAP_FAILED ) );
    TESTSC( ( mmap_array[i] == NULL       ) );
    touch( mmap_array[i] );
  }
}

void ac_eval_munmap( void ) {
  int i;
  for( i=0; i<MMAP_ITER; i++ ) {
    TESTSC( munmap( mmap_array[i], size ) );
  }
}
#endif

void eval_mmap( void ) {
#ifndef ACUM
  par_eval_mmap();
#else
  ac_eval_mmap();
#endif
}

void eval_munmap( void ) {
#ifdef ACUM
  ac_eval_munmap();
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
    TESTINT( pip_spawn( argv[0], argv, NULL, i+1, &pipid, NULL, NULL, NULL ) );
  }

  wait_sync0( &syncs );
  time_start = gettime();
  /* ... */
  wait_sync1( &syncs );
  time_end = gettime();

  //print_maps();

  wait_sync2( &syncs );
  time_start2 = gettime();
  /* ... */
  wait_sync3( &syncs );
  time_end2 = gettime();

  for( i=0; i<ntasks; i++ ) wait( NULL );

  TESTINT( pip_fin() );
}

void eval_pip( char *argv[] ) {
  struct sync_st *syncp;
  void *root_exp;
  int pipid;

  mmap_size( argv[2] );
  TESTINT( pip_init( &pipid, NULL, &root_exp, 0 ) );
  TESTINT( ( pipid == PIP_PIPID_ROOT ) );
  syncp = (struct sync_st*) root_exp;

  wait_sync0( syncp );
  eval_mmap();
  wait_sync1( syncp );

  wait_sync2( syncp );
  eval_munmap();
  wait_sync3( syncp );
#ifdef ACUM
  if(0) {
     int i;
     for( i=0; i<MMAP_ITER; i++ ) {
       printf( "%p   {%d} mmap[%d]\n", mmap_array[i], pipid, i );
     }
  }
#endif
  TESTINT( pip_fin() );
}
#endif

#ifdef THREAD
struct sync_st syncs;

void eval_thread( void ) {
  wait_sync0( &syncs );
  eval_mmap();
  wait_sync1( &syncs );

  wait_sync2( &syncs );
  eval_munmap();
  wait_sync3( &syncs );
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
  time_start = gettime();
  /* ... */
  wait_sync1( &syncs );
  time_end = gettime();

  //print_maps();

  wait_sync2( &syncs );
  time_start2 = gettime();
  /* ... */
  wait_sync3( &syncs );
  time_end2 = gettime();

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
      eval_mmap();
      wait_sync1( syncs );

      wait_sync2( syncs );
      eval_munmap();
      wait_sync3( syncs );
      exit( 0 );
    }
  }
  wait_sync0( syncs );
  time_start = gettime();
  /* ... */
  wait_sync1( syncs );
  time_end = gettime();

  //print_maps();

  wait_sync2( syncs );
  time_start2 = gettime();
  /* ... */
  wait_sync3( syncs );
  time_end2 = gettime();
  for( i=0; i<ntasks; i++ ) wait( NULL );
}
#endif

int main( int argc, char **argv ) {
  int ntasks;

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
    mmap_size( argv[2] );

#if defined( PIP )
    spawn_tasks( argv, ntasks );
#elif defined( THREAD )
    create_threads( ntasks );
#elif defined( FORK_ONLY )
    fork_only( ntasks );
#endif
    printf( "%g [sec], %d tasks, %d iteration, %d [KB] mmap size\n",
	    time_end - time_start, ntasks, MMAP_ITER, ((int)size)/1024 );
#ifdef ACUM
    printf( "%g [sec]--munmap\n", time_end2 - time_start2 );
#endif

  } else {			/* child task/process/thread */
#if defined(PIP)
    eval_pip( argv );
#endif
    printf( "[%d] terminated\n", getpid() );
  }
  return 0;
}
