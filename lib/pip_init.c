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
 * Written by Noriyuki Soda (soda@sra.co.jp) and
 * Atsushi HORI <ahori@riken.jp>, 2018, 2019
 */

#include <pip_internal.h>
#include <pip_gdbif.h>

pip_root_t		*pip_root PIP_PRIVATE;
pip_task_internal_t	*pip_task PIP_PRIVATE;
struct pip_gdbif_root	*pip_gdbif_root PIP_PRIVATE;

pid_t pip_gettid( void ) {
  return (pid_t) syscall( (long int) SYS_gettid );
}

int pip_is_magic_ok( pip_root_t *root ) {
  return strncmp( root->magic,
		  PIP_MAGIC_WORD,
		  PIP_MAGIC_WLEN ) == 0;
}

int pip_is_version_ok( pip_root_t *root ) {
  return( root->version == PIP_API_VERSION );
}

int pip_are_sizes_ok( pip_root_t *root ) {
  return( root->size_root  == sizeof( pip_root_t )          &&
	  root->size_task  == sizeof( pip_task_internal_t ) &&
	  root->size_annex == sizeof( pip_task_annex_t ) );
}

static int pip_check_root( pip_root_t *root ) {
  if( root == NULL ) {
    return 1;
  } else if( !pip_is_magic_ok( root ) ) {
    return 2;
  } else if( !pip_is_version_ok( root ) ) {
    return 3;
  } else if( !pip_are_sizes_ok( root ) ) {
    return 4;
  }
  return 0;
}

int pip_is_threaded_( void ) {
  return pip_root->opts & PIP_MODE_PTHREAD;
}

void pip_message( FILE *fp, char *tagf, const char *format, va_list ap ) {
#define PIP_MESGLEN		(512)
  char mesg[PIP_MESGLEN];
  char idstr[PIP_MIDLEN];
  int len;

  len = pip_idstr( idstr, PIP_MIDLEN );
  len = snprintf( &mesg[0], PIP_MESGLEN-len, tagf, idstr );
  vsnprintf( &mesg[len], PIP_MESGLEN-len, format, ap );
  fprintf( fp, "%s\n", mesg );
  fflush( fp );
}

void pip_info_fmesg( FILE *fp, const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  if( fp == NULL ) fp = stderr;
  pip_message( fp, "PiP-INFO%s ", format, ap );
}

void pip_info_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-INFO%s ", format, ap );
}

void pip_warn_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "PiP-WRN%s ", format, ap );
}

void pip_err_mesg( const char *format, ... ) {
  va_list ap;
  va_start( ap, format );
  pip_message( stderr, "\nPiP-ERR%s ", format, ap );
}

static char *pip_path_gdb;
static char *pip_command_gdb;

pip_task_internal_t *pip_current_task( int tid ) {
  /* do not put any DBG macors in this function */
  pip_root_t		*root = pip_root;
  pip_task_internal_t 	*taski;
  static int		curr = 0;

  if( root != NULL ) {
    if( tid == root->task_root->annex->tid ) {
      return root->task_root;
    } else {
      int i;
      for( i=curr; i<root->ntasks; i++ ) {
	taski = &root->tasks[i];
	if( taski->pipid == PIP_PIPID_ROOT ||
	    ( taski->pipid >= 0 && taski->pipid < root->ntasks ) ) {
	  if( tid == taski->annex->tid ) {
	    curr = i;
	    return taski;
	  }
	}
      }
      for( i=0; i<curr; i++ ) {
	taski = &root->tasks[i];
	if( tid == taski->annex->tid ) {
	  curr = i;
	  return taski;
	}
      }
    }
  }
  return NULL;
}

static int
pip_pipid_str( char *p, size_t sz, int pipid, int upper ) {
  int	n;

  if( 0 <= pipid ) {
    if( upper ) {
      n = snprintf( p, sz, "T%d", pipid );
    } else {
      n = snprintf( p, sz, "t%d", pipid );
    }
  } else if( pipid == PIP_PIPID_ROOT ) {
    if( upper ) {
      n = snprintf( p, sz, "R" );
    } else {
      n = snprintf( p, sz, "r" );
    }
  } else if( pipid == PIP_PIPID_MYSELF ) {
    if( upper ) {
      n = snprintf( p, sz, "S" );
    } else {
      n = snprintf( p, sz, "s" );
    }
  } else if( pipid == PIP_PIPID_ANY ) {
    if( upper ) {
      n = snprintf( p, sz, "A" );
    } else {
      n = snprintf( p, sz, "a" );
    }
  } else if( pipid == PIP_PIPID_NULL ) {
    if( upper ) {
      n = snprintf( p, sz, "N" );
    } else {
      n = snprintf( p, sz, "n" );
    }
  } else {
    n = snprintf( p, sz, "X" );
  }
  return n;
}

int
pip_task_str( char *p, size_t sz, pip_task_internal_t *taski ) {
  int 	n = 0;

  if( taski == NULL ) {
    n = snprintf( p, sz, "_" );
  } else if( taski->type == PIP_TYPE_NONE ) {
    n = snprintf( p, sz, "*" );
  } else if( PIP_ISA_TASK( taski ) ) {
    n = pip_pipid_str( p, sz, taski->pipid, PIP_IS_RUNNING(taski) );
  } else {
    n = snprintf( p, sz, "!" );
  }
  return n;
}

size_t pip_idstr( char *p, size_t s ) {
  pid_t			tid  = pip_gettid();
  pip_task_internal_t	*ctx = pip_task;
  pip_task_internal_t	*kc  = pip_current_task( tid );
  char 			*opn = "[", *cls = "]", *delim = "|";
  int			n;

#ifdef DEBUG
  pip_task_internal_t	*uc  =
    ( kc == NULL ) ? NULL : kc->annex->task_curr;
  char 			*same  = "<";

  pip_task_internal_t	*schd =
    ( uc == NULL ) ? NULL : uc->task_sched;
  char 			*sched = "/";
#endif

  n = snprintf( p, s, "%s", opn ); 	s -= n; p += n;
  {
    n = snprintf( p, s, "%d:", tid ); 	s -= n; p += n;
    n = pip_task_str( p, s, kc ); 	s -= n; p += n;
#ifdef DEBUG
    if( uc != kc || ctx != kc ) {
      n = snprintf( p, s, delim ); 	s -= n; p += n;
      if( uc != kc ) {
	n = pip_task_str( p, s, uc );	s -= n; p += n;
      } else {
	n = snprintf( p, s, same ); 	s -= n; p += n;
      }
      if( uc != schd ) {
	n = snprintf( p, s, sched ); 	s -= n; p += n;
	n = pip_task_str( p, s, schd );	s -= n; p += n;
      }
      n = snprintf( p, s, delim ); 	s -= n; p += n;
      if( ctx == NULL || ctx != uc ) {
	n = pip_task_str( p, s, ctx );	s -= n; p += n;
      } else {
	n = snprintf( p, s, same ); 	s -= n; p += n;
      }
    }
#else
    if( ctx != kc ) {
      n = snprintf( p, s, delim ); 	s -= n; p += n;
      n = pip_task_str( p, s, ctx );	s -= n; p += n;
    }
#endif
  }
  n = snprintf( p, s, "%s", cls ); 	s -= n; p += n;

  return s;
}

void pip_describe( pid_t tid ) {
  pip_task_internal_t *taski = pip_current_task( tid );
  char  *backtrace = "back-traced";
  char  *debugged  = "debugged";
  char	*trailer;

  if( pip_command_gdb == NULL ) {
    trailer = backtrace;
  } else {
    trailer = debugged;
  }
  if( taski != NULL ) {
    int pipid = taski->pipid;
    if( pipid == PIP_PIPID_ROOT ) {
      printf( "\nPiP-ROOT(TID:%d) is %s\n\n", tid, trailer );
    } else if( pipid >= 0 ) {
      printf( "\nPiP-TASK(PIPID:%d,TID:%d) is %s\n\n", pipid, tid, trailer );
    } else {
      printf( "\nPiP-TASK(PIPID:??,TID:%d) is %s\n\n", tid, trailer );
    }
  } else {
    printf( "\nPiP-unknown(TID:%d) is %s\n\n", tid, trailer );
  }
  fflush( NULL );
}

static void pip_attach_gdb( void ) {
  pid_t	target = pip_gettid();
  pid_t	pid;

  if( ( pid = fork() ) == 0 ) {
    extern char **environ;
    char attach[32];
    char describe[64];
    char *argv[32];
    int argc = 0;

    snprintf( attach,   sizeof(attach),   "%d", target );
    snprintf( describe, sizeof(describe), "call pip_describe(%d)", target );
    if( pip_command_gdb == NULL ) {
      /*  1 */ argv[argc++] = pip_path_gdb;
      /*  2 */ argv[argc++] = "-quiet";
      /*  3 */ argv[argc++] = "-ex";
      /*  4 */ argv[argc++] = "set verbose off";
      /*  5 */ argv[argc++] = "-ex";
      /*  6 */ argv[argc++] = "set complaints 0";
      /*  7 */ argv[argc++] = "-ex";
      /*  8 */ argv[argc++] = "set confirm off";
      /*  9 */ argv[argc++] = "-p";
      /* 10 */ argv[argc++] = attach;
      /* 11 */ argv[argc++] = "-ex";
      /* 12 */ argv[argc++] = "info inferiors";
      /* 13 */ argv[argc++] = "-ex";
      /* 14 */ argv[argc++] = describe;
      /* 15 */ argv[argc++] = "-ex";
      /* 16 */ argv[argc++] = "bt";
      /* 17 */ argv[argc++] = "-ex";
      /* 18 */ argv[argc++] = "detach";
      /* 19 */ argv[argc++] = "-ex";
      /* 20 */ argv[argc++] = "quit";
      /* 21 */ argv[argc++] = NULL;
    } else {
      /*  1 */ argv[argc++] = pip_path_gdb;
      /*  2 */ argv[argc++] = "-quiet";
      /*  3 */ argv[argc++] = "-p";
      /*  4 */ argv[argc++] = attach;
      /*  5 */ argv[argc++] = "-x";
      /*  6 */ argv[argc++] = pip_command_gdb;
      /*  7 */ argv[argc++] = "-batch";
      /*  8 */ argv[argc++] = NULL;
    }
    (void) close( 0 );		/* close STDIN */
#ifdef AH
    {
      int i;
      for( i=0; i<argc; i++ ) {
	DBGF( "[%d] %s", i, argv[i] );
      }
    }
#endif
    execve( pip_path_gdb, argv, environ );
    pip_err_mesg( "Failed to execute PiP-gdb (%s)\n", pip_path_gdb );

  } else if( pid < 0 ) {
    pip_err_mesg( "Failed to fork PiP-gdb (%s)\n", pip_path_gdb );
  } else {
    waitpid( pid, NULL, 0 );
  }
}

#define PIP_DEBUG_BUFSZ		(4096)
#define PIP_MAPS_PATH		"/proc/self/maps"

void pip_fprint_maps( FILE *fp ) {
  int fd = open( PIP_MAPS_PATH, O_RDONLY );
  char buf[PIP_DEBUG_BUFSZ];

  if( fd >= 0 ) {
    while( 1 ) {
      ssize_t rc;
      size_t  wc;
      char *p;

      if( ( rc = read( fd, buf, PIP_DEBUG_BUFSZ ) ) <= 0 ) break;
      p = buf;
      do {
	wc = fwrite( p, 1, rc, fp );
	p  += wc;
	rc -= wc;
      } while( rc > 0 );
    }
    close( fd );
  } else {
    pip_err_mesg( "Unable to open %s", PIP_MAPS_PATH );
  }
}

static void pip_show_maps( void ) {
  char *env = getenv( PIP_ENV_SHOW_MAPS );
  if( env != NULL && strcasecmp( env, "on" ) == 0 ) {
    pip_info_mesg( "*** Show MAPS" );
    pip_fprint_maps( stderr );
  }
}

void pip_show_pips( void ) {
  char *env = getenv( PIP_ENV_SHOW_PIPS );
  if( env != NULL && strcasecmp( env, "on" ) == 0 ) {
    if( access( PIP_INSTALL_BIN_PIPS, X_OK ) == 0 ) {
      pip_info_mesg( "*** Show PIPS (%s)", PIP_INSTALL_BIN_PIPS );
      system( PIP_INSTALL_BIN_PIPS );
    } else {
      pip_info_mesg( "*** Show PIPS (%s)", PIP_BUILD_BIN_PIPS );
      system( PIP_BUILD_BIN_PIPS );
    }
  }
}

void pip_debug_info( void ) {
  pip_show_pips();
  pip_show_maps();
  if( pip_path_gdb != NULL ) {
    pip_attach_gdb();
  } else {
    pip_err_mesg( "\tto attach pip-gdb automatically, "
		  "set %s env. to the pip-gdb path\n",
		  PIP_ENV_GDB_PATH );
  }
}

static void pip_exception_handler( int sig, siginfo_t *info, void *extra ) {
  pip_err_mesg( "*** Exception signal: %s (%d) !!\n", strsignal(sig), sig );
  if( pip_root != NULL ) {
    pip_spin_lock( &pip_root->lock_bt );
    pip_debug_info();
    pip_spin_unlock( &pip_root->lock_bt );
  } else {
    pip_debug_info();
  }
  kill( pip_gettid(), sig );
}

static void
pip_add_gdb_signal( sigset_t *sigs, char *token, int len ) {
  DBGF( "token:%*s (%d)", len, token, len );
  if( strncasecmp( "ALL",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGHUP  ) );
    ASSERT( sigaddset( sigs, SIGINT  ) );
    ASSERT( sigaddset( sigs, SIGQUIT ) );
    ASSERT( sigaddset( sigs, SIGILL  ) );
    ASSERT( sigaddset( sigs, SIGABRT ) );
    ASSERT( sigaddset( sigs, SIGFPE  ) );
    ASSERT( sigaddset( sigs, SIGINT  ) );
    ASSERT( sigaddset( sigs, SIGSEGV ) );
    ASSERT( sigaddset( sigs, SIGPIPE ) );
    ASSERT( sigaddset( sigs, SIGUSR1 ) );
    ASSERT( sigaddset( sigs, SIGUSR2 ) );
  } else if( strncasecmp( "HUP",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGHUP ) );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGINT ) );
  } else if( strncasecmp( "QUIT", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGQUIT ) );
  } else if( strncasecmp( "ILL",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGILL ) );
  } else if( strncasecmp( "ABRT", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGABRT ) );
  } else if( strncasecmp( "FPE",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGFPE ) );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGINT ) );
  } else if( strncasecmp( "SEGV", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGSEGV ) );
  } else if( strncasecmp( "PIPE", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGPIPE ) );
  } else if( strncasecmp( "USR1", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGUSR1 ) );
  } else if( strncasecmp( "USR2", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGUSR2 ) );
  }
}

static void
pip_del_gdb_signal( sigset_t *sigs, char *token, int len ) {
  DBGF( "token:%*s (%d)", len, token, len );
  if( strncasecmp( "ALL",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGHUP  ) );
    ASSERT( sigdelset( sigs, SIGINT  ) );
    ASSERT( sigdelset( sigs, SIGQUIT ) );
    ASSERT( sigdelset( sigs, SIGILL  ) );
    ASSERT( sigdelset( sigs, SIGABRT ) );
    ASSERT( sigdelset( sigs, SIGFPE  ) );
    ASSERT( sigdelset( sigs, SIGINT  ) );
    ASSERT( sigdelset( sigs, SIGSEGV ) );
    ASSERT( sigdelset( sigs, SIGPIPE ) );
    ASSERT( sigdelset( sigs, SIGUSR1 ) );
    ASSERT( sigdelset( sigs, SIGUSR2 ) );
  } else if( strncasecmp( "HUP",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGHUP ) );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGINT ) );
  } else if( strncasecmp( "QUIT", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGQUIT ) );
  } else if( strncasecmp( "ILL",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGILL ) );
  } else if( strncasecmp( "ABRT", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGABRT ) );
  } else if( strncasecmp( "FPE",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGFPE ) );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGINT ) );
  } else if( strncasecmp( "SEGV", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGSEGV ) );
  } else if( strncasecmp( "PIPE", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGPIPE ) );
  } else if( strncasecmp( "USR1", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGUSR1 ) );
  } else if( strncasecmp( "USR2", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGUSR2 ) );
  }
}

static int pip_next_token( char *str, int p ) {
  while( str[p] != '\0' &&
	 str[p] != '+'  &&
	 str[p] != '-' ) p ++;
  return p;
}

static void pip_set_gdb_sigset( char *env, sigset_t *sigs ) {
  int i, j;

  DBGF( "signals: %s", env );
  if( ( i = pip_next_token( env, 0 ) ) > 0 ) {
    pip_add_gdb_signal( sigs, env, i );
    while( env[i] != '\0' ) {
      if( env[i] == '+' ) {
	i++;
	j = pip_next_token( env, i );
	if( j > i ) pip_add_gdb_signal( sigs, &env[i], j-i );
	i = j;
      } else if( env[i] == '-' ) {
	i++;
	j = pip_next_token( env, i );
	if( j > i ) pip_del_gdb_signal( sigs, &env[i], j-i );
	i = j;
      }
    }
  }
}

#define ROUNDUP(X,Y)		((((X)+(Y)-1)/(Y))*(Y))

void pip_page_alloc( size_t sz, void **allocp ) {
  size_t pgsz;

  if( pip_root == NULL ) {	/* no pip_root yet */
    pgsz = sysconf( _SC_PAGESIZE );
  } else if( pip_root->page_size == 0 ) {
    pip_root->page_size = pgsz = sysconf( _SC_PAGESIZE );
  } else {
    pgsz = pip_root->page_size;
  }
  sz = ROUNDUP( sz, pgsz );
  ASSERT( posix_memalign( allocp, pgsz, sz ) != 0 &&
	  *allocp == NULL );
}

#define PIP_MINSIGSTKSZ 	(MINSIGSTKSZ*2)

void pip_debug_on_exceptions( pip_task_internal_t *taski ) {
  char			*path, *command, *signals;
  sigset_t 		sigs, sigempty;
  struct sigaction	sigact;
  static int		done = 0;
  int			i;

  if( done ) return;
  done = 1;

  if( ( path = getenv( PIP_ENV_GDB_PATH ) ) != NULL &&
      *path != '\0' ) {
    ASSERT( sigemptyset( &sigs     ) );
    ASSERT( sigemptyset( &sigempty ) );

    if( !pip_is_threaded_() || pip_isa_root() ) {
      if( access( path, X_OK ) != 0 ) {
	  pip_err_mesg( "PiP-gdb unable to execute (%s)\n", path );
      } else {
	pip_path_gdb = path;
	if( ( command = getenv( PIP_ENV_GDB_COMMAND ) ) != NULL &&
	    *command != '\0' ) {
	  if( access( command, R_OK ) == 0 ) pip_command_gdb = command;
	}
	if( ( signals = getenv( PIP_ENV_GDB_SIGNALS ) ) != NULL ) {
	  pip_set_gdb_sigset( signals, &sigs );
	} else {
	  /* default signals */
	  ASSERT( sigaddset( &sigs, SIGHUP  ) );
	  ASSERT( sigaddset( &sigs, SIGSEGV ) );
	}
	if( memcmp( &sigs, &sigempty, sizeof(sigs) ) != 0 ) {
	  /* FIXME: since the sigaltstack is allocated  */
	  /* by a PiP taskthere is no chance to free    */
	  void		*altstack;
	  stack_t	sigstack;

	  pip_page_alloc( PIP_MINSIGSTKSZ, &altstack );
	  taski->annex->sigalt_stack = altstack;
	  memset( &sigstack, 0, sizeof( sigstack ) );
	  sigstack.ss_sp   = altstack;
	  sigstack.ss_size = PIP_MINSIGSTKSZ;
	  ASSERT( sigaltstack( &sigstack, NULL ) );

	  memset( &sigact, 0, sizeof( sigact ) );
	  sigact.sa_sigaction = pip_exception_handler;
	  sigact.sa_mask      = sigs;
	  sigact.sa_flags     = SA_RESETHAND | SA_ONSTACK;

	  for( i=SIGHUP; i<=SIGUSR2; i++ ) {
	    if( sigismember( &sigs, i ) ) {
	      DBGF( "PiP-gdb on signal [%d]: %s ", i, strsignal( i ) );
	      ASSERT( sigaction( i, &sigact, NULL ) );
	    }
	  }
	}
      }
    } else {
      /* exception signals must be blocked in thread mode */
      /* so that root hanlder can catch them */
      if( ( signals = getenv( PIP_ENV_GDB_SIGNALS ) ) != NULL ) {
	pip_set_gdb_sigset( signals, &sigs );
	if( memcmp( &sigs, &sigempty, sizeof(sigs) ) ) {
	  ASSERT( pthread_sigmask( SIG_BLOCK, &sigs, NULL ) );
	}
      } else {
	ASSERT( sigaddset( &sigs, SIGHUP  ) );
	ASSERT( sigaddset( &sigs, SIGSEGV ) );
	ASSERT( pthread_sigmask( SIG_BLOCK, &sigs, NULL ) );
      }
    }
  }
}

/* the following function will be called implicitly */
/* this function is called by PiP tasks only        */
int pip_init_task_implicitly( pip_root_t *root,
			      pip_task_internal_t *task ) {
  int err = pip_check_root( root );
  if( !err ) {
    if( ( pip_root != NULL && pip_root != root ) ||
	( pip_task != NULL && pip_task != task ) ||
	( pip_gdbif_root != NULL &&
	  pip_gdbif_root != root->gdbif_root ) ) {
      err = ELIBSCN;
    } else {
      pip_root = root;
      pip_task = task;
      pip_gdbif_root = root->gdbif_root;
      pip_debug_on_exceptions( task );
    }
  } else {
    err = ELIBSCN;
  }
  return err;
}

/* energy-saving spin-lock */

void pip_glibc_lock( void ) {
  if( pip_root != NULL ) pip_sem_wait( &pip_root->lock_glibc );
}

void pip_glibc_unlock( void ) {
  if( pip_root != NULL ) pip_sem_post( &pip_root->lock_glibc );
}
