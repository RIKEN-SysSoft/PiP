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
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#define _GNU_SOURCE

#include <test.h>
#include <sys/time.h>
#include <sys/wait.h>

#define DEBUG_SCALE	(5)

extern int pip_is_debug_build( void );

static pid_t pid = 0;
static char *prog = NULL;
static char *target = NULL;
static int timer_period = 0;
static int timedout = 0;

static void cleanup( void ) {
  if( pid > 0 ) {
    errno = 0;
    (void) kill( pid, SIGHUP );
    if( errno != ESRCH && target != NULL ) {
      char sysstr[256];
      sleep( 1 );
      sprintf( sysstr, "killall -KILL %s", target );
      system( sysstr );
    }
  }
}

static void timer_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
  timedout = 1;
  /* XXX - the followings are NOT async-signal-safe */
  fprintf( stderr, "Timer expired (%d sec)\n", timer_period );
  fprintf( stderr, "deliver SIGHUP : pid:%d\n", (int) pid );
  cleanup();
  //exit( EXIT_UNRESOLVED );
}

static void set_timer( int timer ) {
  struct sigaction sigact;
  struct itimerval tv;

  timer_period = timer;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = timer_watcher;
  sigact.sa_flags     = SA_RESETHAND;
  if( sigaction( SIGALRM, &sigact, NULL ) != 0 ) {
    fprintf( stderr, "[%s] sigaction(): %d\n", prog, errno );
    cleanup();
    exit( EXIT_UNTESTED );
  }
  memset( &tv, 0, sizeof(tv) );
  tv.it_interval.tv_sec = timer;
  tv.it_value.tv_sec    = timer;
  if( setitimer( ITIMER_REAL, &tv, NULL ) != 0 ) {
    fprintf( stderr, "[%s] setitimer(): %d\n", prog, errno );
    cleanup();
    exit( EXIT_UNTESTED );
  }
}

static void unset_timer( void ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_handler = SIG_IGN;
  if( sigaction( SIGALRM, &sigact, NULL ) != 0 ) {
    fprintf( stderr, "[%s] sigaction(): %d\n", prog, errno );
    cleanup();
  }
}

static void usage( void ) {
  fprintf( stderr, "Usage: %s <time> <prog> [<args>]\n", prog );
  exit( EXIT_UNTESTED );
}

int main( int argc, char **argv ) {
  int 	time, status, flag_debug = 0;

  set_sigsegv_watcher();
  set_sigint_watcher();

  prog = basename( argv[0] );
  if( argc < 3 ) usage();
  target = basename( argv[2] );

  time = strtol( argv[1], NULL, 10 );
  if( time == 0 ) usage();
  if( time <  0 ) {
    time = -time;
    flag_debug = 1;
  }

  if( pip_is_debug_build() && !flag_debug ) {
    time *= DEBUG_SCALE;
  }

  set_timer( time );
  if( ( pid = fork() ) == 0 ) {
    //(void) setpgid( 0, 0 );
    execvp( argv[2], &argv[2] );
    fprintf( stderr, "[%s] execvp(): %d\n", prog, errno );
    exit( EXIT_UNTESTED );

  } else if( pid > 0 ) {
    pid_t rv;
    for( ;; ) {
      rv = wait( &status );
      if (rv != -1 || errno != EINTR) break;
    }
    if (rv == -1) {
      fprintf( stderr, "'%s' failed to wait: %s\n", target, strerror(errno) );
    } else if( WIFEXITED( status ) ) {
      exit( WEXITSTATUS( status ) );
    } else if( WIFSIGNALED( status ) ) {
      int sig = WTERMSIG( status );
      fprintf( stderr, "'%s' terminated due to signal (%s)\n", target, strsignal(sig) );
    }
    exit( EXIT_UNRESOLVED );

  } else {
    fprintf( stderr, "[%s] fork(): %d\n", prog, errno );
    unset_timer();
    exit( EXIT_UNTESTED );
  }
  exit( EXIT_UNTESTED );
}
