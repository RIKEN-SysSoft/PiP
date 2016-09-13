/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
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
#include <string.h>
#include <errno.h>

#include <pip.h>
#include <pip_machdep.h>

#define NTASKS		PIP_NTASKS_MAX

#ifndef DEBUG
//#define DEBUG
#endif

#include <pip_debug.h>

#define PRINT_FL(FSTR,V)	\
  fprintf(stderr,"%s:%d %s=%d\n",__FILE__,__LINE__,FSTR,V)

#ifndef DEBUG

#define TESTINT(F)		\
  do{int __xyz=(F); if(__xyz!=0){PRINT_FL(#F,__xyz);exit(9);}} while(0)

#else

#define PRTNL 			fprintf( stderr, "\n" )
#define TPRT(...)		\
  do { 									\
  int __node = 98765; pip_get_pipid( &__node );				\
  if( __node == 98765 ) {						\
    fprintf( stderr, "<N/A> %s:%d: ", __FILE__, __LINE__ );	\
  } else if( __node == PIP_PIPID_ROOT ) {				\
    fprintf( stderr, "<ROOT> %s:%d: ", __FILE__, __LINE__ ); 		\
  } else {								\
    fprintf( stderr, "<PIP:%d> %s:%d: ", __node, __FILE__, __LINE__ );	\
  } fprintf( stderr, __VA_ARGS__ ); PRTNL; } while(0)
#define TESTINT(F)		\
  do{ 							\
    TPRT( ">> %s", #F );				\
    int __xyz = (F);					\
    TPRT( "<< %s=%d", #F, __xyz );			\
    if( __xyz != 0 ) exit( 9 );				\
  } while(0)
#endif

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
    write( 1, buf, sz );
  }
  close( fd );
}

inline static void print_numa( void ) {
  int fd = open( "/proc/self/numa_maps", O_RDONLY );
  while( 1 ) {
    char buf[1024];
    int sz;
    if( ( sz = read( fd, buf, 1024 ) ) <= 0 ) break;
    write( 1, buf, sz );
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
    fprintf( stderr, "!!!!!! SIGNAL: %s(%d) (pid=%d) !!!!!!\n",
	     signal_name( siginfo->si_signo ),
	     siginfo->si_signo,
	     siginfo->si_pid  );
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

inline static void set_signal_echo( int signal ) {
  void signal_echo( int sig, siginfo_t *siginfo, void *dummy ) {
    fprintf( stderr, "!!!!!! ECHO SIGNAL: %s(%d) (pid=%d) !!!!!!\n",
	     signal_name( siginfo->si_signo ),
	     siginfo->si_signo,
	     siginfo->si_pid  );
    TESTINT( pip_kill( PIP_PIPID_ROOT, siginfo->si_signo ) );
  }
  struct sigaction sigact;
  memset( (void*) &sigact, 0, sizeof( sigact ) );
  sigact.sa_sigaction = signal_echo;
  sigact.sa_flags     = SA_RESETHAND | SA_SIGINFO;
  TESTINT( sigaction( signal, &sigact, NULL ) );
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
    ucontext_t *ctx = (ucontext_t*) context;
    intptr_t pc = (intptr_t) ctx->uc_mcontext.gregs[REG_RIP];
    char *sigcode;
    if( siginfo->si_code == SEGV_MAPERR ) {
      sigcode = "SEGV_MAPERR";
    } else if( siginfo->si_code == SEGV_ACCERR ) {
      sigcode = "SEGV_ACCERR";
    } else {
      sigcode = "(unknown)";
    }
    fprintf( stderr,
	     "!!!!!! SIGSEGV@%p  pid=%d  segvaddr=%p  %s !!!!!!\n",
	     (void*) pc,
	     siginfo->si_pid,
	     siginfo->si_addr,
	     sigcode );
    print_maps();
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

#define NOTWORKING
#ifdef NOTWORKING
#include <pip_internal.h>

inline static void attachme( void ) {
  char *env;

  if( ( env = getenv( PIP_ROOT_ENV ) ) == NULL ) {
    fprintf( stderr, "attachme: unable to attach\n" );
  } else {
    pid_t pid;
    pid_t pid_target = getpid();
    intptr_t	ptr = (intptr_t) strtoll( env, NULL, 16 );
    pip_root_t *pip_root = (pip_root_t*) ptr;
    int pipid;
    int err;

    if( ( err = pip_get_pipid( &pipid ) ) != 0 ) {
    } else if( ( pid = fork() ) == 0 ) {
      char *argv[10];
      char gdbstr[256];
      int i = 0;

      sprintf( gdbstr,
	       "gdb %s %d",
	       pip_root->tasks[pipid].argv[0],
	       pid_target );

      argv[i++] = "sh";
      argv[i++] = "-c";
      argv[i++] = gdbstr;
      argv[i++] = NULL;
      execvp( argv[0], argv );
    }
    wait( NULL );
  }
}
#else
  inline static void attachme( void ) { return; }
#endif


inline static void set_sigint_watcher( void ) {
  void sigint_watcher( int sig, siginfo_t *siginfo, void *context ) {
    ucontext_t *ctx = (ucontext_t*) context;
    intptr_t pc = (intptr_t) ctx->uc_mcontext.gregs[REG_RIP];
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
