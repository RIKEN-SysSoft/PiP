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
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <pip.h>
#include <pip_ulp.h>
#include <pip_universal.h>
#include <pip_util.h>
#include <pip_machdep.h>

#define NTASKS		PIP_NTASKS_MAX

#define EXIT_PASS	0
#define EXIT_FAIL	1
#define EXIT_XPASS	2  /* passed, but it's unexpected. fixed recently?   */
#define EXIT_XFAIL	3  /* failed, but it's expected. i.e. known bug      */
#define EXIT_UNRESOLVED	4  /* cannot determine whether (X)?PASS or (X)?FAIL  */
#define EXIT_UNTESTED	5  /* not tested, this test haven't written yet      */
#define EXIT_UNSUPPORTED 6 /* not tested, this environment can't test this   */
#define EXIT_KILLED	7  /* killed by Control-C or something               */

#ifndef DEBUG
//#define DEBUG
#endif

#include <pip_debug.h>

#define PRINT_FL(FSTR,V)	\
  fprintf(stderr,"%s:%d (%s)=%d\n",__FILE__,__LINE__,FSTR,V)

#ifndef DEBUG

#define TESTINT(F)		\
  do{int __xyz=(F); if(__xyz){PRINT_FL(#F,__xyz);exit(9);}} while(0)
#define TESTSYSERR(F)		\
  do{int __xyz=(F); if(__xyz == -1){PRINT_FL(#F,__xyz);exit(9);}} while(0)
#define TEST_EXPECT(F, X)	\
  do{int __xyz=(F); if(__xyz != X){PRINT_FL(#F,__xyz);exit(9);}} while(0)

#else

#define TPRT(...)		\
  do { char __msgbuf[256]; int n, m = pip_idstr( __msgbuf, 256 );	\
  n = sprintf( &__msgbuf[m], " %s:%d: ", __FILE__, __LINE__ );		\
  sprintf( &__msgbuf[n+m], __VA_ARGS__ );				\
  fprintf( stderr, "%s\n", __msgbuf ); } while(0)
#define TESTINT(F)		\
  do{ 							\
    TPRT( ">> %s", #F );				\
    int __xyz = (F);					\
    TPRT( "<< (%s)=%d", #F, __xyz );			\
    if( __xyz != 0 ) exit( 9 );				\
  } while(0)
#define TESTSYSERR(F)		\
  do{ 							\
    TPRT( ">> %s", #F );				\
    int __xyz = (F);					\
    TPRT( "<< %s=%d", #F, __xyz );			\
    if( __xyz == -1 ) exit( 9 );			\
  } while(0)
#define TEST_EXPECT(F, X)				\
  do{ 							\
    TPRT( ">> %s", #F );				\
    int __xyz = (F);					\
    TPRT( "<< %s=%d", #F, __xyz );			\
    if( __xyz != X ) exit( 9 );				\
  } while(0)
#endif

inline static int cpu_num_limit( void ) {
  static bool initialized = false;
  static long ncpu;

  if( initialized ) {
    return ncpu;
  }

  ncpu = sysconf( _SC_NPROCESSORS_ONLN );
  if( ncpu == -1 ) {
    fprintf( stderr, "sysconf( _SC_NPROCESSORS_ONLN ): %s\n",
	     strerror(errno) );
    exit( EXIT_FAILURE );
  }
  if( ncpu > 16 )
    ncpu = 16;
  initialized = true;
  return ncpu;
}

inline static void pause_and_yield( int usec ) {
  if( usec > 0 ) usleep( usec );
  pip_pause();
  sched_yield();
}

inline static void print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  while( 1 ) {
    char buf[1024];
    int sz;
    if( ( sz = read( fd, buf, 1024 ) ) <= 0 ) break;
    TEST_EXPECT( write( 1, buf, sz ), sz );
  }
  close( fd );
}

inline static void print_numa( void ) {
  int fd = open( "/proc/self/numa_maps", O_RDONLY );
  while( 1 ) {
    char buf[1024];
    int sz;
    if( ( sz = read( fd, buf, 1024 ) ) <= 0 ) break;
    TEST_EXPECT( write( 1, buf, sz ), sz );
  }
  close( fd );
}

#define DUMP_ENV(X,Y)	dump_env( #X, X, Y );
inline static void dump_env( char *tag, char **envv, int nomore ) {
  int i;
  for( i=0; envv[i]!=NULL; i++ ) {
    if( i >= nomore ) {
      fprintf( stderr, "%s[..] more env-vars may follow...\n", tag );
      break;
    }
    fprintf( stderr, "%s[%d] %s\n", tag, i, envv[i] );
  }
}

#ifndef __cplusplus

inline static char *signal_name( int sig ) {
  char *signam_tab[] = {
    "(signal0)",		/* 0 */
    "SIGHUP",			/* 1 */
    "SIGINT",			/* 2 */
    "SIGQUIT",			/* 3 */
    "SIGILL",			/* 4 */
    "SIGTRAP",			/* 5 */
    "SIGABRT",			/* 6 */
    "SIGBUS",			/* 7 */
    "SIGFPE",			/* 8 */
    "SIGKILL",			/* 9 */
    "SIGUSR1",			/* 10 */
    "SIGSEGV",			/* 11 */
    "SIGUSR2",			/* 12 */
    "SIGPIPE",			/* 13 */
    "SIGALRM",			/* 14 */
    "SIGTERM",			/* 15 */
    "SIGSTKFLT",		/* 16 */
    "SIGCHLD",			/* 17 */
    "SIGCONT",			/* 18 */
    "SIGSTOP",			/* 19 */
    "SIGTSTP",			/* 20 */
    "SIGTTIN",			/* 21 */
    "SIGTTOU",			/* 22 */
    "SIGURG",			/* 23 */
    "SIGXCPU",			/* 24 */
    "SIGXFSZ",			/* 25 */
    "SIGVTALRM",		/* 26 */
    "SIGPROF",			/* 27 */
    "SIGWINCH",			/* 28 */
    "SIGIO",			/* 29 */
    "SIGPWR",			/* 30 */
    "SIGSYS",			/* 31 */
    "(not a signal)"
  };
  if( sig < 0 || sig > 31 ) return signam_tab[32];
  return signam_tab[sig];
}

int pip_get_pipid_( void );

inline static void set_signal_watcher( int signal ) {
  void signal_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
    fprintf( stderr,
	     "[PIPID:%d,PID:%d] SIGNAL: %s(%d) addr:%p pid=%d !!!!!!\n",
	     pip_get_pipid_(),
	     getpid(),
	     signal_name( siginfo->si_signo ),
	     siginfo->si_signo,
	     siginfo->si_addr,
	     getpid() );
  }
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = signal_watcher;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  TESTINT( sigaction( signal, &sigact, NULL ) );
}

inline static void ignore_signal( int signal ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_handler = SIG_IGN;
  TESTINT( sigaction( signal, &sigact, NULL ) );
}

inline static void watch_sigchld( void ) {
  set_signal_watcher( SIGCHLD );
}

inline static void watch_anysignal( void ) {
  set_signal_watcher( SIGHUP  );
  set_signal_watcher( SIGINT  );
  set_signal_watcher( SIGQUIT );
  set_signal_watcher( SIGILL  );
  set_signal_watcher( SIGABRT );
  set_signal_watcher( SIGFPE  );
  //set_signal_watcher( SIGKILL );
  set_signal_watcher( SIGSEGV );
  set_signal_watcher( SIGPIPE );
  set_signal_watcher( SIGALRM );
  set_signal_watcher( SIGTERM );
  set_signal_watcher( SIGUSR1 );
  set_signal_watcher( SIGUSR2 );
  set_signal_watcher( SIGCHLD );
  set_signal_watcher( SIGCONT );
  //set_signal_watcher( SIGSTOP );
  set_signal_watcher( SIGTSTP );
  set_signal_watcher( SIGTTIN );
  set_signal_watcher( SIGTTOU );
}

inline static void set_signal_handler( int signal, void(*handler)() ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = handler;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  TESTINT( sigaction( signal, &sigact, NULL ) );
}

static void signal_echo( int sig, siginfo_t *siginfo, void *dummy ) {
  fprintf( stderr, "!!!!!! ECHO SIGNAL: %s(%d) (pid=%d) !!!!!!\n",
	   signal_name( siginfo->si_signo ),
	   siginfo->si_signo,
	   siginfo->si_pid  );
  TESTINT( pip_kill( PIP_PIPID_ROOT, siginfo->si_signo ) );
}

inline static void set_signal_echo( int signal ) {
  set_signal_handler( signal, signal_echo );
}

inline static void echo_anysignal( void ) {
  set_signal_echo( SIGHUP  );
  set_signal_echo( SIGINT  );
  set_signal_echo( SIGQUIT );
  set_signal_echo( SIGILL  );
  set_signal_echo( SIGABRT );
  set_signal_echo( SIGFPE  );
  //set_signal_echo( SIGKILL );
  set_signal_echo( SIGSEGV );
  set_signal_echo( SIGPIPE );
  set_signal_echo( SIGALRM );
  set_signal_echo( SIGTERM );
  set_signal_echo( SIGUSR1 );
  set_signal_echo( SIGUSR2 );
  set_signal_echo( SIGCHLD );
  set_signal_echo( SIGCONT );
  //set_signal_echo( SIGSTOP );
  set_signal_echo( SIGTSTP );
  set_signal_echo( SIGTTIN );
  set_signal_echo( SIGTTOU );
}

inline static void ignore_anysignal( void ) {
  ignore_signal( SIGHUP  );
  ignore_signal( SIGINT  );
  ignore_signal( SIGQUIT );
  ignore_signal( SIGILL  );
  ignore_signal( SIGABRT );
  ignore_signal( SIGFPE  );
  //ignore_signal( SIGKILL );
  ignore_signal( SIGSEGV );
  ignore_signal( SIGPIPE );
  ignore_signal( SIGALRM );
  ignore_signal( SIGTERM );
  ignore_signal( SIGUSR1 );
  ignore_signal( SIGUSR2 );
  ignore_signal( SIGCHLD );
  ignore_signal( SIGCONT );
  //ignore_signal( SIGSTOP );
  ignore_signal( SIGTSTP );
  ignore_signal( SIGTTIN );
  ignore_signal( SIGTTOU );
}

inline static void set_sigsegv_watcher( void ) {
  void sigsegv_watcher( int sig, siginfo_t *siginfo, void *context ) {
#ifdef REG_RIP
    ucontext_t *ctx = (ucontext_t*) context;
    intptr_t pc = (intptr_t) ctx->uc_mcontext.gregs[REG_RIP];
#else
    intptr_t pc = 0;
#endif
    char *sigcode;
    if( siginfo->si_code == SEGV_MAPERR ) {
      sigcode = "SEGV_MAPERR";
    } else if( siginfo->si_code == SEGV_ACCERR ) {
      sigcode = "SEGV_ACCERR";
    } else {
      sigcode = "(unknown)";
    }
    fflush( NULL );
    fprintf( stderr,
	     "\n"
	     "[PIPID:%d,PID:%d] SIGSEGV@%p  pid=%d  segvaddr=%p  %s !!!!!!\n"
	     "\n",
	     pip_get_pipid_(),
	     getpid(),
	     (void*) pc,
	     siginfo->si_pid,
	     siginfo->si_addr,
	     sigcode );
    //print_maps();
  }

  struct sigaction sigact;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = sigsegv_watcher;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  TESTINT( sigaction( SIGSEGV, &sigact, NULL ) );
}

#define PROCFD		"/proc/self/fd"
inline static int print_fds( FILE *file ) {
  struct dirent **entry_list;
  int count;
  int err = 0;

  int filter( const struct dirent *entry ) {
    return entry->d_type == DT_LNK;
  }

  if( ( count = scandir( PROCFD, &entry_list, filter, alphasort ) ) < 0 ) {
    err = errno;
  } else {
    char pipidstr[64];
    int pipid;
    int i;

    (void) pip_get_pipid( &pipid );
    if( pipid == PIP_PIPID_ROOT ) {
      sprintf( pipidstr, "ROOT" );
    } else {
      sprintf( pipidstr, "%d", pipid );
    }
    for( i=0; i<count; i++ ) {
      struct dirent *entry = entry_list[i];
      char path[512];
      char name[512];
      int cc;

      sprintf( path, "%s/%s", PROCFD, entry->d_name );
      free(entry);
      if( ( cc = readlink( path, name, 512 ) ) < 0 ) {
	err = errno;
	break;
      }
      name[cc] = '\0';
      fprintf( file, "[%s] %s -> %s\n", pipidstr, path, name );
    }
    free( entry_list );
  }
  return err;
}

inline static void set_sigint_watcher( void ) {
  void sigint_watcher( int sig, siginfo_t *siginfo, void *context ) {
#ifdef REG_RIP
    ucontext_t *ctx = (ucontext_t*) context;
    intptr_t pc = (intptr_t) ctx->uc_mcontext.gregs[REG_RIP];
#else
    intptr_t pc = 0;
#endif
    fprintf( stderr,
	     "\n...... SIGINT@%p  pid=%d  segvaddr=%p !!!!!!\n",
	     (void*) pc,
	     siginfo->si_pid,
	     siginfo->si_addr );
    print_maps();
    while( 1 );
  }

  struct sigaction sigact;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = sigint_watcher;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  TESTINT( sigaction( SIGINT, &sigact, NULL ) );
}

#endif

unsigned long get_total_memory( void ) {
  FILE *fp;
  int ns = 0;
  unsigned long memtotal = 0;

  if( ( fp = fopen( "/proc/meminfo", "r" ) ) != NULL ) {
    ns = fscanf( fp, "MemTotal: %lu", &memtotal );
    fclose( fp );
  }
  if ( ns > 0 ) return memtotal;
  return 0;
}
