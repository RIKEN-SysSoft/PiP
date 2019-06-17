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
#include <time.h>

//#define DEBUG

#include <test.h>

extern int test_main( exp_t *exp );

int pip_id;

static pid_t pid_root;

static void print_usage( void ) {
  fprintf( stderr, "  -- USAGE --\n" );
  fprintf( stderr, "    prog <Nsec> <Nactives> <Npassives> <Niterations>\n" );
  fprintf( stderr, "    ENV: PIPTEST_WAITANY: is set, pip_wait_any() is used\n" );
  fprintf( stderr, "    ENV: %s: to select PiP execution mode\n",
	   PIP_ENV_MODE );
  fprintf( stderr, "    ENV: %s: to select PiP-BLT synchronization method\n",
	   PIP_ENV_SYNC );
  exit( 1 );
}

static void print_conditions( char *prefix,
			      test_args_t *argsp,
			      int timer,
			      char *env_wait ) {
  time_t tm;
  char *waitstr;
  char now[128];

  time( &tm );
  strftime( now, 128, "%m/%d %T", (const struct tm*) localtime( &tm ) );

  if( env_wait ) {
    waitstr = "WAIT_ANY";
  } else {
    waitstr = "WAIT";
  }
  fprintf( stderr,
	   "%s %s %s  timeout:%d[sec.]  ntasks:%d(%d+%d)  niters:%d -- %s\n",
	   prefix,
	   now,
	   argsp->argv[0],
	   timer,
	   argsp->ntasks,
	   argsp->nact,
	   argsp->npass,
	   argsp->niters,
	   waitstr );
}

#define TIMER_DEFAULT 	(10)	/* 10 sec */

static void set_timer( int timer ) {
  extern void pip_abort_all_tasks( void );
  struct sigaction sigact;
  void timer_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
    fflush( NULL );
    fprintf( stderr, "Timer expired !!!! (%d Sec)\n", timer );
    system( "ps uxw" );
    killpg( pid_root, SIGKILL );
    exit( EXIT_UNRESOLVED );
  }
  struct itimerval tv;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = timer_watcher;
  sigact.sa_flags     = SA_RESETHAND;
  TESTINT( sigaction( SIGALRM, &sigact, NULL ) );

  memset( &tv, 0, sizeof(tv) );
  tv.it_interval.tv_sec = timer;
  tv.it_value.tv_sec    = timer;
  TESTINT( setitimer( ITIMER_REAL, &tv, NULL ) );
}

static void unset_timer( void ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_handler = SIG_IGN;
  TESTINT( sigaction( SIGALRM, &sigact, NULL ) );
}

int task_start( void *args ) {
  exp_t 		*exp = (exp_t*) args;
  pip_task_queue_t 	*queue = &exp->queue;
  int			pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  naive_barrier_wait( &exp->nbarr[0] );
#ifdef DEBUG
  fprintf( stderr, "[%d] barrier (0)!!\n", pipid );
#endif

  pip_id = pipid;
  if( pipid < exp->args.npass ) {
    TESTINT( pip_suspend_and_enqueue( queue, NULL, NULL ) );
    TESTINT( test_main( exp ) );

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
    TESTINT( test_main( exp ) );
  }
  TESTINT( pip_fin() );

  return pipid & EXTVAL_MASK;
}

static int set_params( test_args_t *args, int argc, char **argv ) {
  char	*env_ntasks = getenv( "NTASKS" );
  int	ntasks = 0, nact = 0, npass = 0, niters = 0;
  int	ntasks_max;
  int	timer = TIMER_DEFAULT;

  if( env_ntasks != NULL ) {
    ntasks_max = strtol( env_ntasks, NULL, 10 );
  } else {
    ntasks_max = NTASKS;
  }
  if( argc > 1 ) {
    if( !isdigit( *argv[1] ) ) print_usage();
    timer = strtol( argv[1], NULL, 10 );
    if( timer == 0 ) timer = TIMER_DEFAULT;
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

  if( argc > 4 ) {
    niters = strtol( argv[4], NULL, 10 );
  }
  if( pip_is_debug_build() ) {
    niters /= 10;
    if( niters == 0 ) niters = 1;
  }
  if( niters == 0 ) niters = 10;

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

static int check_status( int id, int st, char *wait ) {
  if( WIFEXITED( st ) ) {
    if( WEXITSTATUS( st ) != ( id & EXTVAL_MASK ) ) {
      fprintf( stderr, "[%d] exited 0x%x (%d!=%d) -- %s\n",
	       id, st, st, id & EXTVAL_MASK, wait );
      return -1;
    }
  } else if( WIFSIGNALED( st ) ) {
    int sig = WTERMSIG( st );
    fprintf( stderr, "[%d] (signal '%s':%d) -- %s\n",
	     id, strsignal(sig), sig, wait );
    return sig;
  }
  return 0;
}

static void kill_all_tasks( int ntasks ) {
  int i;
  for( i=0; i<ntasks; i++ ) pip_kill( i, SIGKILL );
}

static int root_proc( int argc, char **argv ) {
  pip_spawn_program_t prog;
  char	*env_wait = getenv( "PIPTEST_WAITANY" );
  exp_t	*exp;
  int 	timer, pipid, ntasks, i, err=0;
  int 	id, st;

  set_sigsegv_watcher();

  exp = set_export( argc, argv, &timer );
  set_timer( timer );
  ntasks = exp->args.ntasks;
  TESTINT( pip_init( NULL, &ntasks, (void**) &exp, 0 ) );
  print_conditions( ">>", &exp->args, timer, env_wait );


  pip_spawn_from_func( &prog, argv[0], "task_start", (void*) exp, NULL );
  for( i=0; i<ntasks; i++ ) {
    char env[128];
    sprintf( env, "%s=%d", PIPENV, i );
    putenv( env );
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, 0, &pipid, NULL ) );
  }
  if( env_wait ) {
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait_any( &id, &st ) );
      if( ( err = check_status( id, st, "waitany" ) ) != 0 ) break;
    }
  } else {
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, &st ) );
      if( ( err = check_status( i, st, "wait" ) ) != 0 ) break;
    }
  }
  unset_timer();
  //fprintf( stderr, "ERR:%d\n", err );
  if( err ) kill_all_tasks( ntasks );
  TESTINT( pip_fin() );

  print_conditions( "<<", &exp->args, timer, env_wait );
  if( err < 0 ) {
    printf( "FAILED!!\n" );
    _exit( EXIT_FAIL );
  } else if( err > 0 ) {
    printf( "FAILED!! (signal '%s':%d)\n", strsignal( err ), err );
    _exit( EXIT_FAIL );
  } else {
    printf( "Success\n" );
    _exit( EXIT_PASS );
  }
}

int main( int argc, char **argv ) {
  void sigint_handler( int sig, siginfo_t *siginfo, void *dummy ) {
    int status;
    fprintf( stderr, "Interrupted by SIGINT !!!!!\n" );
    (void) killpg( pid_root, SIGINT );
    (void) wait( &status );
    exit( EXIT_KILLED );
  }
  struct sigaction sigact;
  int status, sig, extval = 0;

  if( ( pid_root = fork() ) == 0 ) {
    (void) setpgid( 0, 0 );
    extval = root_proc( argc, argv );
    exit( extval );

  } else if( pid_root > 0 ) {
    memset( (void*) &sigact, 0, sizeof( sigact ) );
    sigact.sa_sigaction = sigint_handler;
    sigact.sa_flags     = SA_RESETHAND;
    TESTINT( sigaction( SIGINT, &sigact, NULL ) );

    pid_t pid = wait( &status );
    if( WIFEXITED( status ) ) {
      extval = WEXITSTATUS( status );
    } else if( WIFSIGNALED( status ) ) {
      char *msg = NULL;
      sig = WTERMSIG( status );
      switch( sig ) {
      case SIGQUIT: /* an error was detected and aborted */
	extval = EXIT_XFAIL;
	msg = "Aborted due to an error";
	break;
      case SIGKILL: /* timed out */
	extval = EXIT_UNRESOLVED;
	msg = "Execution timed out";
	break;
      case SIGHUP: /* deadlock detected */
	extval = EXIT_XFAIL;
	msg = "Deadlocked";
	break;
      case SIGINT: /* ^C was hit */
	extval = EXIT_KILLED;
	msg = "Killed by user";
	break;
      default:			/* SEGV ? */
	extval = EXIT_XFAIL;
	break;
      }
      if( msg == NULL ) {
	fprintf( stderr,
		 "<<PID:%d>> Test program terminated with '%s' (%d) !!!!\n",
		 pid, strsignal( sig ), sig );
      } else {
	fprintf( stderr, "%s\n", msg );
      }
    }
  } else {
    fprintf( stderr, "Fork failed (%d)\n", errno );
  }
  return extval;
}
