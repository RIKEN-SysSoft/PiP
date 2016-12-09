// Copyright (C) 2010  Benoit Sigoure
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <linux/futex.h>

#include <eval.h>

#ifdef PIP
#include <pip.h>
#endif

#ifdef PTHREAD
#include <pthread.h>
#endif

static inline long long unsigned time_ns(struct timespec* const ts) {
  if (clock_gettime(CLOCK_REALTIME, ts)) {
    exit(1);
  }
  return ((long long unsigned) ts->tv_sec) * 1000000000LLU
    + (long long unsigned) ts->tv_nsec;
}

static inline int get_iterations(int ws_pages) {
  int iterations = 1000;
  while (iterations * ws_pages * 4096L < 4294967296L) {  // 4GB
    iterations += 1000;
  }
  return iterations;
}

long ws_pages;
void* ws;
int iterations;
struct timespec ts;
long long unsigned memset_time = 0;
int* futex;

void eval_memset( int argc, char **argv ) {
  int i;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <size of working set in 4K pages>\n", *argv);
    exit( 1 );
  }
  ws_pages = strtol(argv[1], NULL, 10);
  if (ws_pages < 0) {
    fprintf(stderr, "Invalid usage: working set size must be positive\n");
    exit( 1 );
  }
  iterations = get_iterations(ws_pages);

  if (ws_pages) {
    void* buf;

    posix_memalign( &buf, 4096, ws_pages * 4096);
    memset_time = time_ns(&ts);
    for (i = 0; i < iterations; i++) {
      memset(buf, i, ws_pages * 4096);
    }
    memset_time = time_ns(&ts) - memset_time;
    printf("%i memset on %4liK in %10lluns (%.1fns/page)\n",
           iterations, ws_pages * 4, memset_time,
           (memset_time / ((float) ws_pages * iterations)));
    free(buf);
  }
}

void eval_task( void ) {
  int i;

  for (i = 0; i < iterations; i++) {
    sched_yield();
    while (syscall(SYS_futex, futex, FUTEX_WAIT, 0xA, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
    *futex = 0xB;
    if (ws_pages) {
      memset(ws, i, ws_pages * 4096);
    }
    while (!syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
  }
}

void eval_main( void ) {
  int i;
  const long long unsigned start_ns = time_ns(&ts);
  for (i = 0; i < iterations; i++) {
    *futex = 0xA;
    if (ws_pages) {
      memset(ws, i, ws_pages * 4096);
    }
    while (!syscall(SYS_futex, futex, FUTEX_WAKE, 1, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
    sched_yield();
    while (syscall(SYS_futex, futex, FUTEX_WAIT, 0xB, NULL, NULL, 42)) {
      // retry
      sched_yield();
    }
  }
  const long long unsigned delta = time_ns(&ts) - start_ns - memset_time * 2;
  const int nswitches = iterations * 4;
  printf("%i process context switches (wss:%4liK) in %12lluns (%.1f ns/ctxsw)\n",
	 nswitches, ws_pages * 4, delta, (delta / (float) nswitches));
  wait(futex);
}

void alloc_futex( void ) {
  void *mmapp;
  mmapp = mmap( NULL,
		(ws_pages + 1) * 4096,
		PROT_READ|PROT_WRITE,
		MAP_SHARED|MAP_ANONYMOUS,
		-1,
		0 );
  TESTSC( ( mmapp == MAP_FAILED ) );
  futex = (int*) mmapp;
  *futex = 0xA;
  ws = ((char *) futex) + 4096;
}

int main(int argc, char** argv) {

#if defined(PIP)
  int pipid, ntasks;

  ntasks = 10;
  TESTINT( pip_init( &pipid, &ntasks, NULL, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {
    eval_memset( argc, argv );
    alloc_futex();
    TESTINT( pip_export( futex ) );
    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, 0, &pipid, NULL, NULL, NULL ) );
    eval_main();
  } else {
    TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &futex ) );
    ws_pages = strtol(argv[1], NULL, 10);
    iterations = get_iterations(ws_pages);
    ws = ((char *) futex) + 4096;
    eval_task();
  }

#elif defined(THREAD)
  pthread_t thread;
  pthread_attr_t attr;

  eval_memset( argc, argv );
  alloc_futex();

  TESTINT( pthread_attr_init( &attr ) );
  TESTINT( pthread_create( &thread,
			   &attr,
			   (void*(*)(void*))eval_task,
			   NULL ) );
  eval_main();

#elif defined(FORK_ONLY)
  pid_t pid;

  eval_memset( argc, argv );
  alloc_futex();

  if( ( pid = fork() ) > 0 ) {
    eval_main();
  } else {
    eval_task();
  }
#endif

  return 0;
}
