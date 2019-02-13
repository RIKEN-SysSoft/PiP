/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016-2019
 */

//#define DEBUG
#include <test.h>

#include <ctype.h>
#include <pthread.h>

int	pipid, ntasks=0, nact=0, npass=0, niters=0;

static char 	*argv0, *waitstr, *syncstr;

#define EXTVAL_MASK	(0xF)

extern int task_main( int, char** );

#define TIMER_DEFAULT 	(1)	/* 1 sec */
static int timer = 0;

static void print_conditions( void ) {
  fprintf( stderr, "[%s] timer:%d  ntasks:%d(%d+%d)  niters:%d -- %s %s\n",
	   argv0, timer, ntasks, nact, npass, niters, waitstr, syncstr );
}

static void set_timer( void ) {
  extern void pip_abort_all_tasks( void );
  struct sigaction sigact;
  void timer_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
    fprintf( stderr,"Timer expired !!!! (%d Sec)\n", timer );
    print_conditions();
    pip_abort_all_tasks();
    exit( EXIT_UNRESOLVED );
  }
  struct itimerval tv;

  if( timer == 0 ) timer = TIMER_DEFAULT;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = timer_watcher;
  sigact.sa_flags     = SA_RESETHAND;
  TESTINT( sigaction( SIGALRM, &sigact, NULL ) );

  memset( &tv, 0, sizeof(tv) );
  tv.it_interval.tv_sec = timer;
  tv.it_value.tv_sec    = timer;
  TESTINT( setitimer( ITIMER_REAL, &tv, NULL ) );
}

void *set_export( void ) __attribute__((weak));
void *set_export( void ) { return (void*) &comm; }

int check_args( void )  __attribute__((weak));
int check_args( void ) { return 0; }

static void print_usage( void ) {
  fprintf( stderr, "  -- USAGE --\n" );
  fprintf( stderr, "    prog <Nsec> <Nactives> <Npassives> <Niterations>\n" );
  fprintf( stderr, "    ENV: PIPTEST_WAIT: to select wait method\n" );
  fprintf( stderr, "    ENV: %s: to select PiP execution mode\n",
	   PIP_ENV_MODE );
  fprintf( stderr, "    ENV: %s: to select PiP-BLT synchronization method\n",
	   PIP_ENV_SYNC );
  exit( 1 );
}

int main( int argc, char **argv ) {
  char	*env_ntasks = getenv( "NTASKS" );
  char	*env_wait   = getenv( "PIPTEST_WAITANY" );
  char	*env_sync   = getenv( PIP_ENV_SYNC );
  void 	*exp;
  int 	ntasks_max, i, err=0;

  set_sigsegv_watcher();

  if( env_ntasks != NULL ) {
    ntasks_max = strtol( env_ntasks, NULL, 10 );
  } else {
    ntasks_max = NTASKS;
  }

  if( argc > 1 ) {
    if( !isdigit( *argv[1] ) ) print_usage();
    timer = strtol( argv[1], NULL, 10 );
  }
  if( argc > 2 ) {
    if( *argv[2] == '*' ) {
      nact = -1;
    } else {
      nact = strtol( argv[2], NULL, 10 );
      if( nact > NTASKS ) {
	fprintf( stderr, "nact(%d) is larger than %d\n", nact, NTASKS );
      }
    }
  }
  if( argc > 3 ) {
    if( *argv[3] == '*' ) {
      npass = -1;
    } else {
      npass = strtol( argv[3], NULL, 10 );
      if( npass > NTASKS ) {
	fprintf( stderr, "npass(%d) is larger than %d\n", npass, NTASKS );
      }
    }
  }
  if( nact < 0 && npass < 0 ) {
    print_usage();
  } else   if( nact > 0 && npass < 0 ) {
    npass = ntasks_max - nact;
  } else if( nact < 0 && npass > 0 ) {
    nact  = ntasks_max - npass;
  } else if( ( nact + npass ) > ntasks_max ) {
    fprintf( stderr, "nact(%d)+npass(%d) is larger than %d\n",
	     nact, npass, ntasks_max );
    exit( 1 );
  }
  ntasks = nact + npass;
  if( ntasks == 0 ) {
    ntasks = nact = ntasks_max;
  }

  if( argc > 4 ) {
    niters = strtol( argv[4], NULL, 10 );
  }

  if( check_args() != 0 ) exit( 1 );

  argv0 = argv[0];
  if( env_wait ) {
    waitstr = "WAIT_ANY";
  } else {
    waitstr = "WAIT";
  }
  if( env_sync ) {
    syncstr = env_sync;
  } else {
    syncstr = "(default:SYNC_AUTO)";
  }

  exp = set_export();
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    print_conditions();
    set_timer();
    pip_task_queue_init( &comm.queue, NULL, NULL );
    comm.npass = 0;
    naive_barrier_init( &comm.nbarr[0], ntasks );
    naive_barrier_init( &comm.nbarr[1], ntasks-npass );

    pip_barrier_init( &comm.pbarr, ntasks );
    pip_mutex_init( &comm.pmutex );

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ) );
    }
    if( env_wait ) {
      for( i=0; i<ntasks; i++ ) {
	int id, st;
	TESTINT( pip_wait_any( &id, &st ) );
	if( st != ( id & EXTVAL_MASK ) ) {
	  err = 1;
	  fprintf( stderr, "[%d] exited 0x%x (%d!=%d) -- waitany\n",
		   id, st, st, id & EXTVAL_MASK );
	}
      }
    } else {
      for( i=0; i<ntasks; i++ ) {
	int st;
	TESTINT( pip_wait( i, &st ) );
	if( st != ( i & EXTVAL_MASK ) ) {
	  err = 1;
	  fprintf( stderr, "[%d] exited 0x%x (%d!=%d) -- wait\n",
		   i, st, st, i & EXTVAL_MASK );
	}
      }
    }
    print_conditions();
    if( err ) {
      printf( "FAILED!!\n" );
      exit( EXIT_FAIL );
    } else {
      printf( "Success\n" );
      exit( EXIT_PASS );
    }

  } else {
    struct comm 	*commp = (struct comm*) exp;
    pip_task_queue_t 	*queue = &commp->queue;

    naive_barrier_wait( &commp->nbarr[0] );
#ifdef DEBUG
    fprintf( stderr, "[%d] barrier (0)!!\n", pipid );
#endif

    if( pipid < npass ) {
      TESTINT( pip_suspend_and_enqueue( queue, NULL, NULL ) );
      TESTINT( task_main( argc, argv ) );
    } else {
      do {
	err = pip_dequeue_and_resume( queue, pip_task_self() );
	if( !err )  {
	  (void) pip_atomic_fetch_and_add( &commp->npass, 1 );
	} else if( err != ENOENT ) {
	  TESTINT( err );
	}
      } while( commp->npass < npass );

      naive_barrier_wait( &commp->nbarr[1] );
#ifdef DEBUG
      fprintf( stderr, "[%d] barrier (1)!!\n", pipid );
#endif
      TESTINT( task_main( argc, argv ) );
    }
  }
  TESTINT( pip_fin() );
  return pipid & EXTVAL_MASK;
}
