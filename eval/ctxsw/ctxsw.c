/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the FreeBSD Project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/time.h>
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

//#define CTXSW_ITER	(100*1000)
#define CTXSW_ITER	(1000)

#define CACHEBLKSZ	(64)

#define CORENO		(0)

//#define COUNT_CTXSW

#ifdef COUNT_CTXSW
#if defined( PIP ) || defined( FORK_ONLY )
pid_t        pid_prev;
#else
pthread_t pthread_prev;
#endif
#endif

__thread int        count_ctxsw;

size_t        ncacheblk = 0;
double        time_start, time_end;

struct sync_st {
  int        count_ctxsw;
  volatile int        sync0;
  volatile int        sync1;
};

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
}

#define ctxsw	sched_yield

#ifdef AHAHA
void ctxsw( void ) {
#if defined( PIP ) || defined( FORK_ONLY )
#ifdef COUNT_CTXSW
    pid_prev = getpid();
#endif
    sched_yield();
#ifdef COUNT_CTXSW
    if( pid_prev == getpid() ) (*count_ctxswp) ++;
#endif

#elif defined( THREAD )
#ifdef COUNT_CTXSW
    pthread_prev = pthread_self();
#endif
    pthread_yield();

#ifdef COUNT_CTXSW
    if( pthread_prev == pthread_self() ) (*count_ctxswp) ++;
#endif
#endif
}
#endif

void wait_sync( volatile int *syncp ) {
  __sync_fetch_and_sub( syncp, 1 );
  while( *syncp > 0 ) ctxsw();
}

void wait_sync0( struct sync_st *syncs ) {
  wait_sync( &syncs->sync0 );
}

void wait_sync1( struct sync_st *syncs ) {
  wait_sync( &syncs->sync1 );
}

void touch( void *p ) {
  if( p != NULL ) {
    int i;
    for( i=0; i<ncacheblk; i++ ) {
      *((int*) p) = i;
      p += CACHEBLKSZ;
    }
  }
}

void *alloc_mem( void ) {
  void *mem;

  if( ncacheblk > 0 ) {
    posix_memalign( &mem, 4096, ncacheblk * CACHEBLKSZ );
    if( mem == NULL ) {
      fprintf( stderr, "not enough memory\n" );
    }
    return mem;
  } else {
    return NULL;
  }
}

void eval_ctxsw( void *mem, int *count_ctxswp ) {
  int i;

  for( i=0; i<CTXSW_ITER; i++ ) {
    touch( mem );
    ctxsw();
  }
}

#ifdef PIP

struct sync_st syncs;

void spawn_tasks( char **argv, int ntasks ) {
  void *root_exp;
  int pipid, i;
  struct rusage ru;
  int nctxsw = 0;

  init_syncs( ntasks+1, &syncs );
#ifdef COUNT_CTXSW
  syncs.count_ctxsw = 0;
#endif
  root_exp = (void*) &syncs;
  TESTINT( pip_init( &pipid, &ntasks, &root_exp, 0 ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );
  argv[1] = "0";
  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_spawn( argv[0], argv, NULL, CORENO, &pipid, NULL, NULL, NULL ) );
  }
  wait_sync0( &syncs );
  time_start = gettime();
  /* ... */
  wait_sync1( &syncs );
  time_end = gettime();

  for( i=0; i<ntasks; i++ ) pip_wait( i, NULL );
}

void eval_pip( char *argv[] ) {
  struct sync_st *syncp;
  void *root_exp;
  void *mem = alloc_mem();
  int pipid;

  ncacheblk = atoi( argv[2] );
  TESTINT( pip_init( &pipid, NULL, &root_exp, 0 ) );
  TESTINT( ( pipid == PIP_PIPID_ROOT ) );
  syncp = (struct sync_st*) root_exp;
  wait_sync0( syncp );
  eval_ctxsw( mem, &syncp->count_ctxsw );
  wait_sync1( syncp );

#ifdef COUNT_CTXSW
  fprintf( stderr, "[%d] nctxsw - %d\n", getpid(), syncp->count_ctxsw );
#endif
  free( mem );
}
#endif

#ifdef THREAD

struct sync_st syncs;

void eval_thread( void ) {
  void *mem = alloc_mem();

#ifdef COUNT_CTXSW
  count_ctxsw = 0;
#endif
  wait_sync0( &syncs );
  eval_ctxsw( mem, &count_ctxsw );
  wait_sync1( &syncs );

#ifdef COUNT_CTXSW
  fprintf( stderr, "[%d] nctxsw - %d\n", getpid(), count_ctxsw );
#endif
  free( mem );

  sleep( 3 );
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
    CPU_SET( CORENO, &cpuset );
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
  struct rusage ru;
  int nctxsw = 0;

  mmapp = mmap( NULL,
		4096,
		PROT_READ|PROT_WRITE,
		MAP_SHARED|MAP_ANONYMOUS,
		-1,
		0 );
  TESTSC( ( mmapp == MAP_FAILED ) );

  syncs = (struct sync_st*) mmapp;
  init_syncs( ntasks+1, syncs );
#ifdef COUNT_CTXSW
  syncs->count_ctxsw = 0;
#endif

  for( i=0; i<ntasks; i++ ) {
    if( ( pid = fork() ) == 0 ) {
      void *mem = alloc_mem();
      cpu_set_t cpuset;

      CPU_ZERO( &cpuset );
      CPU_SET( CORENO, &cpuset );
      TESTSC( sched_setaffinity( getpid(), sizeof(cpuset), &cpuset ) );
      sched_yield();

      wait_sync0( syncs );
      eval_ctxsw( mem, &syncs->count_ctxsw );
      wait_sync1( syncs );

#ifdef COUNT_CTXSW
      fprintf( stderr, "[%d] nctxsw - %d\n", getpid(), syncs->count_ctxsw );
#endif

      exit( 0 );
    }
  }
  wait_sync0( syncs );
  time_start = gettime();
  /* ... */
  wait_sync1( syncs );
  time_end = gettime();

  for( i=0; i<ntasks; i++ ) wait( NULL );
}
#endif

int main( int argc, char **argv ) {
  int ntasks;

  if( argc < 3 ) {
    fprintf( stderr, "ctxsw-XXX <ntasks> <ncacheblks>\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );
  ncacheblk = atoi( argv[2] );

  if( ntasks > 0 ) {		/* root process/thread */
    if( ntasks > NTASKS_MAX ) {
      fprintf( stderr, "Illegal number of tasks is specified.\n" );
      exit( 1 );
    }
#ifdef COUNT_CTXSW
    count_ctxsw = 0;
#endif

#if defined( PIP )
    spawn_tasks( argv, ntasks );
#elif defined( THREAD )
    create_threads( ntasks );
#elif defined( FORK_ONLY )
    fork_only( ntasks );
#endif
    printf( ",%g, [sec]  %d tasks  %d [Bytes] %d iter.\n",
	    time_end - time_start, ntasks,
	    ((int) ( ncacheblk * CACHEBLKSZ )), CTXSW_ITER );

  } else {			/* child task/process/thread */
#if defined(PIP)
    eval_pip( argv );
#endif
  }
#if defined(PIP)
  TESTINT( pip_fin() );
#endif
  return 0;
}
