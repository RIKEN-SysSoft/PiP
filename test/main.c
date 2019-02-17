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

#include <sys/wait.h>
#include <ctype.h>

#define DEBUG
#include <test.h>

extern int test_main( void );

int check_args( void )  __attribute__((weak));
int check_args( void ) { return 0; }

#define EXTVAL_MASK	(0xF)

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

static void print_conditions( test_args_t *argsp, int timer, char *env_wait ) {
  char *waitstr;

  if( env_wait ) {
    waitstr = "WAIT_ANY";
  } else {
    waitstr = "WAIT";
  }
  fprintf( stderr, "[%s] timer:%d  ntasks:%d(%d+%d)  niters:%d -- %s\n",
	   argsp->argv[0],
	   timer,
	   argsp->ntasks,
	   argsp->nact,
	   argsp->npass,
	   argsp->niters,
	   waitstr );
}

#define TIMER_DEFAULT 	(1)	/* 1 sec */

static void set_timer( int timer ) {
  extern void pip_abort_all_tasks( void );
  struct sigaction sigact;
  void timer_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
    fprintf( stderr, "Timer expired !!!! (%d Sec)\n", timer );
    killpg( getpid(), SIGKILL );
    exit( EXIT_UNRESOLVED );
  }
  struct itimerval tv;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = timer_watcher;
  sigact.sa_flags     = SA_RESETHAND;
  TESTINT( sigaction( SIGALRM, &sigact, NULL ) );

  if( timer == 0 ) timer = TIMER_DEFAULT;
  memset( &tv, 0, sizeof(tv) );
  tv.it_interval.tv_sec = timer;
  tv.it_value.tv_sec    = timer;
  TESTINT( setitimer( ITIMER_REAL, &tv, NULL ) );
}

int task_start( void *args ) {
  exp_t *exp = (exp_t*) args;
  pip_task_queue_t 	*queue = &exp->queue;
  int			pipid;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  naive_barrier_wait( &exp->nbarr[0] );
#ifdef DEBUG
  fprintf( stderr, "[%d] barrier (0)!!\n", pipid );
#endif

  if( pipid < exp->args.npass ) {
    TESTINT( pip_suspend_and_enqueue( queue, NULL, NULL ) );
    TESTINT( test_main() );
  } else {
    do {
      int err = pip_dequeue_and_resume( queue, pip_task_self() );
      if( !err )  {
	(void) pip_atomic_fetch_and_add( &exp->npass, 1 );
      } else if( err != ENOENT ) {
	TESTINT( err );
      }
    } while( exp->npass < exp->args.npass );

    naive_barrier_wait( &exp->nbarr[1] );
#ifdef DEBUG
    fprintf( stderr, "[%d] barrier (1)!!\n", pipid );
#endif
    TESTINT( test_main() );
  }
  TESTINT( pip_fin() );

  return pipid & EXTVAL_MASK;
}

static int set_params( test_args_t *args, int argc, char **argv ) {
  char	*env_ntasks = getenv( "NTASKS" );
  int	ntasks = 0, nact = 0, npass = 0, niters = 1;
  int	ntasks_max, timer = TIMER_DEFAULT;

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
  if( ntasks == 0 ) ntasks = nact = ntasks_max;

  if( argc > 4 ) niters = strtol( argv[4], NULL, 10 );
  if( check_args() != 0 ) exit( 1 );

  args->argc   = argc;
  args->argv   = argv;
  args->ntasks = ntasks;
  args->nact   = nact;
  args->npass  = npass;
  args->niters = niters;

  return timer;
}

void *set_export( int argc, char **argv, int *timerp ) __attribute__((weak));
void *set_export( int argc, char **argv, int *timerp ) {
  static exp_t exp;

  memset( &exp, 0, sizeof( exp_t ) );
  *timerp = set_params( &exp.args, argc, argv );

  pip_task_queue_init( &exp.queue, NULL );
  exp.npass = 0;
  naive_barrier_init( &exp.nbarr[0], exp.args.ntasks );
  naive_barrier_init( &exp.nbarr[1], exp.args.ntasks - exp.args.npass );

  pip_barrier_init( &exp.pbarr, exp.args.ntasks );
  pip_mutex_init( &exp.pmutex );

  return (void*) &exp;
}

int root_proc( int argc, char **argv ) {
  pip_spawn_program_t prog;
  char	*env_wait = getenv( "PIPTEST_WAITANY" );
  exp_t	*exp;
  int 	timer, pipid, ntasks, i, err=0;

  set_sigsegv_watcher();

  exp = set_export( argc, argv, &timer );
  set_timer( timer );
  ntasks = exp->args.ntasks;
  TESTINT( pip_init( NULL, &ntasks, (void**) &exp, 0 ) );
  print_conditions( &exp->args, timer, env_wait );

  pip_spawn_from_func( &prog, argv[0], "task_start", (void*) exp, NULL );
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
  TESTINT( pip_fin() );

  print_conditions( &exp->args, timer, env_wait );
  if( err ) {
    printf( "FAILED!!\n" );
    exit( EXIT_FAIL );
  } else {
    printf( "Success\n" );
    exit( EXIT_PASS );
  }
}

int main( int argc, char **argv ) {
  pid_t pid;
  int status, sig, extval = 0;

  if( ( pid = fork() ) == 0 ) {
    (void) setpgid( 0, getppid() );
    extval = root_proc( argc, argv );
    exit( extval );
  } else if( pid > 0 ) {
    (void) wait( &status );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
    } else if( WIFSIGNALED( status ) ) {
      sig = WTERMSIG( status );
      extval = EXIT_XFAIL;
      fprintf( stderr,
	       "!!!! Test program terminated with '%s' !!!!\n",
	       strsignal( sig ) );
    }
  } else {
    fprintf( stderr, "Fork failed (%d)\n", errno );
  }
  return extval;
}
