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

/** \addtogroup piprun piprun
 *
 * \brief run programs as PiP tasks
 *
 * \section synopsis SYNOPSIS
 *
 *	\c \b piprun [OPTIONS] &lt;program&gt; ... [ :: ... ]
 *
 * \section description DESCRIPTION
 * \b Run a program as PiP task(s).  Mutiple programs can be specified
 * by separating them with '::'.
 *
 * -n \b &lt;N&gt; number of tasks\n
 * -f \b &lt;FUNC&gt; function name to start\n
 * -c \b &lt;CORE&gt; specify the CPU core number to bind core(s)\n
 * -r core binding in the round-robin fashion\n
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <pip.h>

//#define DEBUG

#define COREBIND_RR	(-100)

typedef struct arg {
  struct arg	*next;
  char		*arg;
} arg_t;

typedef struct spawn {
  struct spawn	*next;
  int		coreno;
  int		ntasks;
  int		argc;
  char		*func;
  arg_t		*args;
  arg_t		*tail;
} spawn_t;

static void print_usage( void ) {
  fprintf( stderr,
	   "Usage: %s [-n N] [-c C] [-f F] A.OUT ... "
	   "{ :: [-n N] [-c C] [-f F] B.OUT ... } \n",
	   PROGRAM );
  fprintf( stderr, "\t-n N\t: Number of PiP tasks (default is 1)\n" );
  fprintf( stderr, "\t-f F\t: Function name in the program to start\n" );
  fprintf( stderr, "\t-c C\t: CPU core number to bind PiP task(s), or\n" );
  fprintf( stderr, "\t-r\t: CPU cores are bound in the round-robin fashion\n");
  exit( 1 );
}

static int count_cpu( void ) {
  cpu_set_t cpuset;
  int c = -1;

  if( sched_getaffinity( getpid(), sizeof(cpuset), &cpuset ) == 0 ) {
    c = CPU_COUNT( &cpuset );
  }
  return c;
}

static arg_t *new_arg( char *a ) {
  arg_t	*arg = (arg_t*) malloc( sizeof( arg_t ) );
  if( arg == NULL ) {
    fprintf( stderr, "Not enough memory (arg)\n" );
    exit( 9 );
  }
  memset( arg, 0, sizeof( arg_t ) );
  arg->arg = a;
  return arg;
}

static spawn_t *new_spawn( void ) {
  spawn_t *spawn = (spawn_t*) malloc( sizeof( spawn_t ) );
  if( spawn == NULL ) {
    fprintf( stderr, "Not enough memory (spawn)\n" );
    exit( 9 );
  }
  memset( spawn, 0, sizeof( spawn_t ) );
  spawn->ntasks = 1;
  spawn->coreno = PIP_CPUCORE_ASIS;
  return spawn;
}

static void free_spawn( spawn_t *spawn ) {
  void free_arg( arg_t *arg ) {
    if( arg == NULL ) return;
    free_arg( arg->next );
    free( arg );
  }
  if( spawn == NULL ) return;
  free_spawn( spawn->next );
  free_arg( spawn->args->next );
  free( spawn );
}

static int nth_core( int coreno ) {
  cpu_set_t cpuset;
  int i, c = 0;

  if( sched_getaffinity( getpid(), sizeof(cpuset), &cpuset ) == 0 ) {
    for( i=0; i<coreno+1; i++ ) {
      if( CPU_ISSET( i, &cpuset ) ) c = i;
    }
  }
  return c;
}

int main( int argc, char **argv ) {
  spawn_t	*spawn, *head, *tail;
  arg_t		*arg;
  char		**nargv = NULL;
  int pipid  = 0;
  int opts   = 0;
  int ntasks;
  int argc_max;
  int cn, ncores = count_cpu();
  int i, err = 0;

  if( argc < 2 || argv[1] == NULL ) print_usage();

  head = tail = NULL;
  i = 1;
  for( ; i<argc; i++ ) {
    spawn = new_spawn();
    if( head == NULL ) head = spawn;
    if( tail != NULL ) tail->next = spawn;
    tail = spawn;
    for( ; i<argc; i++ ) {
      if( strcmp( argv[i], "::" ) == 0 ) {
	if( spawn->args == NULL ) print_usage();
	break;
      } else if( *argv[i] != '-' ) {
	if( access( argv[i], X_OK ) ) {
	  err = errno;
	  fprintf( stderr, "Unable to execute '%s'\n", argv[i] );
	  goto error;
	}
	spawn->args = spawn->tail = new_arg( argv[i++] );
	spawn->argc = 1;
	for( ; i<argc; i++ ) {
	  if( strcmp( argv[i], "::" ) == 0 ) break;
	  arg = new_arg( argv[i] );
	  spawn->tail->next = arg;
	  spawn->tail = arg;
	  spawn->argc ++;
	}
	break;
      } else if( strcmp( argv[i], "-h"     ) == 0 ||
		 strcmp( argv[i], "--help" ) == 0  ) {
	print_usage();
      } else if( strcmp( argv[i], "-n" ) == 0 ) {
	if( argv[i+1] == NULL ||
	    !isdigit( *argv[i+1] ) ||
	    ( spawn->ntasks = atoi( argv[i+1] ) ) == 0 ) {
	  print_usage();
	}
	i ++;
      } else if( strcmp( argv[i], "-c" ) == 0 ) {
	if( argv[i+1] == NULL || !isdigit( *argv[i+1] ) ) {
	  print_usage();
	}
	spawn->coreno = atoi( argv[++i] ) % ncores;
      } else if( strcmp( argv[i], "-b" ) == 0 || /* deprecated option */
		 strcmp( argv[i], "-r" ) == 0 ) {
	spawn->coreno = COREBIND_RR;
      } else if( strcmp( argv[i], "-f" ) == 0 && argv[i+1] != NULL ) {
	spawn->func = argv[++i];
      } else {
	print_usage();
      }
    }
  }

#ifdef DEBUG
  for( spawn = head, i = 0; spawn != NULL; spawn = spawn->next ) {
    printf( "[%d] ntasks:%d coreno:%d argc:%d func:'%s'\n",
	    i++, spawn->ntasks, spawn->coreno, spawn->argc, spawn->func );
    for( arg = spawn->args, j = 0; arg != NULL; arg = arg->next ) {
      printf( "    (%d) '%s'\n", j++, arg->arg );
    }
  }
#endif

  ntasks   = 0;
  argc_max = 0;
  for( spawn = head; spawn != NULL; spawn = spawn->next ) {
    if( spawn->args == NULL || spawn->args->arg == NULL ) {
      print_usage();
    }
    ntasks += spawn->ntasks;
    argc_max = ( spawn->argc > argc_max ) ? spawn->argc : argc_max;
  }
  argc_max ++;
  nargv = (char**) malloc( sizeof( char* ) * argc_max );
  if( nargv == NULL ) {
    fprintf( stderr, "Not enough memory (nargv)\n" );
    err = ENOMEM;
    goto error;
  }

  if( ( err = pip_init( NULL, &ntasks, NULL, opts ) ) != 0 ) {
    fprintf( stderr, "pip_init()=%d\n", err );
    return err;
  }
  cn = 0;
  for( spawn = head; spawn != NULL; spawn = spawn->next ) {
    int c;

    for( arg = spawn->args, i = 0; arg != NULL; arg = arg->next ) {
      nargv[i++] = arg->arg;
    }
    nargv[i] = NULL;
    for( i=0; i<spawn->ntasks; i++ ) {
      if( spawn->coreno == PIP_CPUCORE_ASIS ) {
	c = spawn->coreno;
      } else if( spawn->coreno == COREBIND_RR ) {
	c = nth_core( ( cn++ ) % ncores );
      } else {
	c = nth_core( ( spawn->coreno + i ) % ncores );
      }
      pipid = i;
      err = pip_spawn( nargv[0],
		       nargv,
		       NULL,
		       c,
		       &pipid,
		       NULL,
		       NULL,
		       NULL );
      if( err ) pip_abort();
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      while( pip_wait( i, &status ) < 0 ) {
	if( errno == ECHILD ) break;
      }
    }
  }
 error:
  if( nargv != NULL ) free( nargv );
  free_spawn( head );
  return err;
}

#endif
