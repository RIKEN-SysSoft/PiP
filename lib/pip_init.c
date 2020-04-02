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
  fprintf( stderr, "%s\n", mesg );
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

void pip_describe( pid_t tid ) {
  pip_task_internal_t *taski;
  int   i;

  DBGF( "tid:%d", tid );
  if( pip_root != NULL ) {
    if( tid == pip_root->task_root->annex->tid ) {
      if( pip_command_gdb == NULL ) {
	printf( "<PiP-ROOT(%d)> is back-traced\n", tid );
      } else {
	printf( "<PiP-ROOT(%d)> is debugged\n", tid );
      }
      goto done;
    } else {
      for( i=0; i<pip_root->ntasks; i++ ) {
	taski = &pip_root->tasks[i];
	DBGF( "task-pipid:%d  TID:%d", taski->pipid, taski->annex->tid );
	if( tid == taski->annex->tid ) {
	  if( pip_command_gdb == NULL ) {
	    printf( "<PiP-TASK(%d)> is back-traced\n", tid );
	  } else {
	    printf( "<PiP-TASK(%d)> is debugged\n", tid );
	  }
	  goto done;
	}
      }
    }
    if( pip_command_gdb == NULL ) {
      printf( "<PiP-unknown(%d)> is back-traced\n", tid );
    } else {
      printf( "<PiP-unknown(%d)> is debugged\n", tid );
    }
  }
 done:
  fflush( NULL );
}

void pip_attach_gdb( void ) {
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
      argv[argc++] = pip_path_gdb;
      argv[argc++] = "-quiet";
      argv[argc++] = "-p";
      argv[argc++] = attach;
      argv[argc++] = "-ex";
      argv[argc++] = "set verbose off";
      argv[argc++] = "-ex";
      argv[argc++] = "set complaints 0";
      argv[argc++] = "-ex";
      argv[argc++] = "set confirm off";
      argv[argc++] = "-ex";
      argv[argc++] = describe;
      argv[argc++] = "-ex";
      argv[argc++] = "bt";
      argv[argc++] = "-ex";
      argv[argc++] = "detach";
      argv[argc++] = "-ex";
      argv[argc++] = "quit";
      argv[argc++] = NULL;
    } else {
      argv[argc++] = pip_path_gdb;
      argv[argc++] = "-quiet";
      argv[argc++] = "-p";
      argv[argc++] = attach;
      argv[argc++] = "-x";
      argv[argc++] = pip_command_gdb;
      argv[argc++] = "-batch";
      argv[argc++] = NULL;
    }
    (void) close( 0 );
#ifdef DEBUG
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

void pip_print_maps( void ) { pip_fprint_maps( stdout ); }

static void pip_print_maps_on_exception( void ) {
  char *env = getenv( PIP_ENV_SHOW_MAPS );
  if( env != NULL && strcasecmp( env, "on" ) == 0 ) {
    pip_fprint_maps( stderr );
  }
}

static void pip_exception_handler( int sig, siginfo_t *info, void *extra ) {
  if( pip_path_gdb != NULL ) {
    if( pip_root != NULL ) {
      pip_spin_lock( &pip_root->lock_bt );
      pip_err_mesg( "Exception signal: %s (%d) !!\n", strsignal(sig), sig );
      pip_print_maps_on_exception();
      pip_attach_gdb();
      pip_spin_unlock( &pip_root->lock_bt );
    } else {
      pip_err_mesg( "Exception signal: %s (%d) !!\n", strsignal(sig), sig );
      pip_print_maps_on_exception();
      pip_attach_gdb();
    }
  } else {
    if( pip_root != NULL ) {
      pip_spin_lock( &pip_root->lock_bt );
      pip_print_maps_on_exception();
      pip_err_mesg( "Exception signal: %s (%d) !!\n"
		    "\tto attach pip-gdb automatically, "
		    "set %s env. to the pip-gdb path\n",
		    strsignal(sig), sig, PIP_ENV_GDB_PATH );
      pip_spin_unlock( &pip_root->lock_bt );
    } else {
      pip_print_maps_on_exception();
      pip_err_mesg( "Exception signal: %s (%d) !!\n"
		    "\tto attach pip-gdb automatically, "
		    "set %s env. to the pip-gdb path\n",
		    strsignal(sig), sig, PIP_ENV_GDB_PATH );
    }
  }
  kill( pip_gettid(), sig );
}

static void
pip_add_gdb_signal( sigset_t *sigs, char *token, int len ) {
  DBGF( "token:%*s (%d)", len, token, len );
  if( strncasecmp( "ALL",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGHUP  ) != 0 );
    ASSERT( sigaddset( sigs, SIGINT  ) != 0 );
    ASSERT( sigaddset( sigs, SIGQUIT ) != 0 );
    ASSERT( sigaddset( sigs, SIGILL  ) != 0 );
    ASSERT( sigaddset( sigs, SIGABRT ) != 0 );
    ASSERT( sigaddset( sigs, SIGFPE  ) != 0 );
    ASSERT( sigaddset( sigs, SIGINT  ) != 0 );
    ASSERT( sigaddset( sigs, SIGSEGV ) != 0 );
    ASSERT( sigaddset( sigs, SIGPIPE ) != 0 );
    ASSERT( sigaddset( sigs, SIGUSR1 ) != 0 );
    ASSERT( sigaddset( sigs, SIGUSR2 ) != 0 );
  } else if( strncasecmp( "HUP",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGHUP ) != 0 );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGINT ) != 0 );
  } else if( strncasecmp( "QUIT", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGQUIT ) != 0 );
  } else if( strncasecmp( "ILL",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGILL ) != 0 );
  } else if( strncasecmp( "ABRT", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGABRT ) != 0 );
  } else if( strncasecmp( "FPE",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGFPE ) != 0 );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGINT ) != 0 );
  } else if( strncasecmp( "SEGV", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGSEGV ) != 0 );
  } else if( strncasecmp( "PIPE", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGPIPE ) != 0 );
  } else if( strncasecmp( "USR1", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGUSR1 ) != 0 );
  } else if( strncasecmp( "USR2", token, len ) == 0 ) {
    ASSERT( sigaddset( sigs, SIGUSR2 ) != 0 );
  }
}

static void
pip_del_gdb_signal( sigset_t *sigs, char *token, int len ) {
  DBGF( "token:%*s (%d)", len, token, len );
  if( strncasecmp( "ALL",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGHUP  ) != 0 );
    ASSERT( sigdelset( sigs, SIGINT  ) != 0 );
    ASSERT( sigdelset( sigs, SIGQUIT ) != 0 );
    ASSERT( sigdelset( sigs, SIGILL  ) != 0 );
    ASSERT( sigdelset( sigs, SIGABRT ) != 0 );
    ASSERT( sigdelset( sigs, SIGFPE  ) != 0 );
    ASSERT( sigdelset( sigs, SIGINT  ) != 0 );
    ASSERT( sigdelset( sigs, SIGSEGV ) != 0 );
    ASSERT( sigdelset( sigs, SIGPIPE ) != 0 );
    ASSERT( sigdelset( sigs, SIGUSR1 ) != 0 );
    ASSERT( sigdelset( sigs, SIGUSR2 ) != 0 );
  } else if( strncasecmp( "HUP",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGHUP ) != 0 );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGINT ) != 0 );
  } else if( strncasecmp( "QUIT", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGQUIT ) != 0 );
  } else if( strncasecmp( "ILL",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGILL ) != 0 );
  } else if( strncasecmp( "ABRT", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGABRT ) != 0 );
  } else if( strncasecmp( "FPE",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGFPE ) != 0 );
  } else if( strncasecmp( "INT",  token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGINT ) != 0 );
  } else if( strncasecmp( "SEGV", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGSEGV ) != 0 );
  } else if( strncasecmp( "PIPE", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGPIPE ) != 0 );
  } else if( strncasecmp( "USR1", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGUSR1 ) != 0 );
  } else if( strncasecmp( "USR2", token, len ) == 0 ) {
    ASSERT( sigdelset( sigs, SIGUSR2 ) != 0 );
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

void pip_debug_on_exceptions( void ) {
  char			*path, *command, *signals;
  sigset_t 		sigs, sigempty;
  struct sigaction	sigact;
  int			i;

  ASSERT( sigemptyset( &sigs )     != 0 );
  ASSERT( sigemptyset( &sigempty ) != 0 );

  if( pip_is_threaded_() && !pip_isa_root() ) {
    if( ( path = getenv( PIP_ENV_GDB_PATH ) ) != NULL &&
	*path != '\0' ) {
      if( ( signals = getenv( PIP_ENV_GDB_SIGNALS ) ) != NULL ) {
	pip_set_gdb_sigset( signals, &sigs );
	if( memcmp( &sigs, &sigempty, sizeof(sigs) ) != 0 ) {
	  ASSERT( pthread_sigmask( SIG_BLOCK, &sigs, NULL ) );
	}
      } else {
	/* exception signals must be blocked   */
	/* so that root hanlder can catch them */
	ASSERT( sigaddset( &sigs, SIGHUP  ) != 0 );
	ASSERT( sigaddset( &sigs, SIGSEGV ) != 0 );
	ASSERT( pthread_sigmask( SIG_BLOCK, &sigs, NULL ) );
      }
    }
  } else {
    if( ( path = getenv( PIP_ENV_GDB_PATH ) ) != NULL &&
	*path != '\0' ) {
      if( access( path, X_OK ) == 0 ) {
	pip_path_gdb = path;
	if( ( command = getenv( PIP_ENV_GDB_COMMAND ) ) != NULL &&
	    *command != '\0' ) {
	  if( access( command, R_OK ) == 0 ) {
	    pip_command_gdb = command;
	  }
	}
      } else {
	pip_err_mesg( "PiP-gdb unable to execute (%s)\n", path );
      }
    }
    if( ( signals = getenv( PIP_ENV_GDB_SIGNALS ) ) != NULL ) {
      pip_set_gdb_sigset( signals, &sigs );
    } else {
      /* default signals */
      ASSERT( sigaddset( &sigs, SIGHUP  ) != 0 );
      ASSERT( sigaddset( &sigs, SIGSEGV ) != 0 );
    }
    if( memcmp( &sigs, &sigempty, sizeof(sigs) ) != 0 ) {
      memset( &sigact, 0, sizeof( sigact ) );
      sigact.sa_sigaction = pip_exception_handler;
      sigact.sa_flags     = SA_RESETHAND;
      for( i=1; i<SIGUSR2; i++ ) {
	if( sigismember( &sigs, i ) ) {
	  DBGF( "PiP-gdb on signal [%d]: %s ", i, strsignal( i ) );
	  ASSERT( sigaction( i, &sigact, NULL ) != 0 );
	}
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
      err = ENXIO;
    } else {
      pip_root = root;
      pip_task = task;
      pip_gdbif_root = root->gdbif_root;
      pip_debug_on_exceptions();
    }
  }
  return err;
}

/* energy-saving spin-lock */

void pip_glibc_lock( int nsec100 ) {
  if( pip_root != NULL ) {
    if( nsec100 > 0 ) {
      while( !pip_spin_trylock( &pip_root->lock_glibc ) ) {
	usleep( nsec100 * 100 );
      }
    } else {
      while( !pip_spin_trylock( &pip_root->lock_glibc ) ) {
	pip_pause();
      }
    }
  }
}

void pip_glibc_unlock( void ) {
  if( pip_root != NULL ) {
    pip_spin_unlock( &pip_root->lock_glibc );
  }
}
