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

#ifndef __test_h__
#define __test_h__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#define __USE_GNU
#include <dlfcn.h>
#include <stdbool.h>

#include <pip.h>
#include <pip_blt.h>
#include <pip_internal.h>

#define NTASKS			PIP_NTASKS_MAX
#define EXTVAL_MASK		(127)

#define PIPENV			"PIPENV"

#define PIP_TEST_NTASKS_ENV 	"PIP_TEST_NTASKS"
#define PIP_TEST_PIPID_ENV 	"PIP_TEST_PIPID"

#define EXIT_PASS	0
#define EXIT_FAIL	1
#define EXIT_XPASS	2  /* passed, but it's unexpected. fixed recently?   */
#define EXIT_XFAIL	3  /* failed, but it's expected. i.e. known bug      */
#define EXIT_UNRESOLVED	4  /* cannot determine whether (X)?PASS or (X)?FAIL  */
#define EXIT_UNTESTED	5  /* not tested, this test haven't written yet      */
#define EXIT_UNSUPPORTED 6 /* not tested, this environment can't test this   */
#define EXIT_KILLED	7  /* killed by Control-C or something               */

extern int pip_id;

typedef struct naive_barrier {
  struct {
    int			count_init;
    pip_atomic_t        count;
    volatile int        gsense;
  };
} naive_barrier_t;

typedef struct test_args {
  int 		argc;
  char 		**argv;
  int		ntasks;
  int		nact;
  int		npass;
  int		timer;
  volatile int	niters;
} test_args_t;

typedef struct exp {
  test_args_t		args;
  naive_barrier_t	nbarr[2];
  pip_task_queue_t 	queue;
  pip_atomic_t		npass;
  pip_barrier_t		pbarr;
  pip_mutex_t		pmutex;
  volatile int		tmp;
} exp_t;

inline static int naive_barrier_init( naive_barrier_t *barrp, int n ) {
  if( n < 1 ) RETURN( EINVAL );
  barrp->count      = n;
  barrp->count_init = n;
  barrp->gsense     = 0;
  return( 0 );
}

inline static int naive_barrier_wait( naive_barrier_t *barrp ) {
  if( barrp->count_init > 1 ) {
    int lsense = !barrp->gsense;
    if( pip_atomic_sub_and_fetch( &barrp->count, 1 ) == 0 ) {
      barrp->count  = barrp->count_init;
      pip_memory_barrier();
      barrp->gsense = lsense;
    } else {
      while( barrp->gsense != lsense ) {
	pip_yield( PIP_YIELD_DEFAULT );
      }
    }
  }
  return 0;
}

typedef pip_spinlock_t		naive_lock_t;

inline static void naive_lock( naive_lock_t *lock ) {
  while( !pip_spin_trylock( lock ) ) {
    pip_yield( PIP_YIELD_DEFAULT );
  }
}

inline static void naive_unlock( naive_lock_t *lock ) {
  *lock = 0;
}

inline static void naive_lock_init( naive_lock_t *lock ) {
  naive_unlock( lock );
}

inline static pid_t my_gettid( void ) {
  return (pid_t) syscall( (long int) SYS_gettid );
}

#define TRUE		(1)
#define FALSE		(0)

extern char *__progname;

#define PRINT_FL(FSTR,VAL)	\
  fprintf(stderr,"%s:%d (%s)=%d\n",__FILE__,__LINE__,FSTR,VAL)

#define PRINT_FLE(FSTR,ERR)			\
  if(!errno) {								\
    fprintf(stderr,"\n[%s(%d)] %s:%d (%s): %s (%ld)\n\n",		\
	    __progname,my_gettid(),__FILE__,__LINE__,FSTR,strerror(ERR),ERR); \
  } else {								\
    fprintf(stderr,"\n[%s(%d)] %s:%d (%s): %s (errno: %s)\n\n",		\
	    __progname, my_gettid(), __FILE__,__LINE__,FSTR,		\
	    strerror(ERR), strerror(errno) ); }

#ifndef DEBUG
#define CHECK(F,C,A) \
  do{ errno=0; long int RV=(intptr_t)(F);	\
    if(C) { PRINT_FLE(#F,RV); A; } } while(0)
#else
#define CHECK(F,C,A)							\
  do{									\
    fprintf(stderr,"[%s(%d)] %s:%d %s: >> %s\n",__progname,my_gettid(),	\
	    __FILE__,__LINE__,__func__,#F );				\
    errno=0; long int RV=(intptr_t)(F);					\
    fprintf(stderr,"[%s(%d)] %s:%d %s: << %s\n",__progname,my_gettid(),	\
	    __FILE__,__LINE__,__func__,#F );				\
    if(C) { PRINT_FLE(#F,RV); A; }					\
  } while(0)
#endif

inline static void pause_and_yield( int usec ) {
  if( usec > 0 ) usleep( usec );
  pip_yield( PIP_YIELD_DEFAULT );
}

inline static void print_maps( void ) {
  int fd = open( "/proc/self/maps", O_RDONLY );
  while( 1 ) {
    char buf[1024];
    int sz;
    if( ( sz = read( fd, buf, 1024 ) ) <= 0 ) break;
    CHECK( write( 1, buf, sz ), RV<0, pip_abort() );
  }
  close( fd );
}

inline static void print_numa( void ) {
  int fd = open( "/proc/self/numa_maps", O_RDONLY );
  while( 1 ) {
    char buf[1024];
    int sz;
    if( ( sz = read( fd, buf, 1024 ) ) <= 0 ) break;
    CHECK( write( 1, buf, sz ), RV<0, pip_abort() );
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

inline static void set_signal_watcher( int signal ) {
  void signal_watcher( int sig, siginfo_t *siginfo, void *dummy ) {
    char idstr[64];

    pip_idstr( idstr, 64 );
    fprintf( stderr,
	     "%s SIGNAL: %s(%d) addr:%p pid=%d !!!!!!\n",
	     idstr,
	     signal_name( siginfo->si_signo ),
	     siginfo->si_signo,
	     siginfo->si_addr,
	     my_gettid() );
  }
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = signal_watcher;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  CHECK( sigaction( signal, &sigact, NULL ), RV, pip_abort() );
}

inline static void ignore_signal( int signal ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_handler = SIG_IGN;
  CHECK( sigaction( signal, &sigact, NULL ), RV, pip_abort() );
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

inline static int set_signal_handler( int signal, void(*handler)() ) {
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = handler;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  errno = 0;
  (void) sigaction( signal, &sigact, NULL );
  return errno;
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
    char idstr[64];

    pip_idstr( idstr, 64 );
    if( siginfo->si_code == SEGV_MAPERR ) {
      sigcode = "SEGV_MAPERR";
    } else if( siginfo->si_code == SEGV_ACCERR ) {
      sigcode = "SEGV_ACCERR";
    } else {
      sigcode = "(unknown)";
    }
    fprintf( stderr,
	     "\n"
	     "%s SIGSEGV  pc:%p  pid:%d  segvaddr:%p  '%s' !!!!!!\n"
	     "\n",
	     idstr,
	     (void*) pc,
	     siginfo->si_pid,
	     siginfo->si_addr,
	     sigcode );
    pip_backtrace_fd( 0, 2 );	/* fflush is called inside */
    pip_abort();
  }
  struct sigaction sigact;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = sigsegv_watcher;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  CHECK( sigaction( SIGSEGV, &sigact, NULL ), RV, pip_abort() );
}

inline static void set_sigint_watcher( void ) {
  void sigint_watcher( int sig, siginfo_t *info, void* extra ) {
    char idstr[64];

    pip_idstr( idstr, 64 );
    fprintf( stderr, "\n%s interrupted by user !!!!!!\n", idstr );
    fflush( NULL );
    exit( EXIT_UNRESOLVED );
  }

  struct sigaction sigact;

  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = sigint_watcher;
  sigact.sa_flags     = SA_RESETHAND;
  CHECK( sigaction( SIGINT, &sigact, NULL ), RV, pip_abort() );
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
#endif

inline static unsigned long get_total_memory( void ) {
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

extern int pip_is_debug_build( void );

#endif /* __test_h__ */
