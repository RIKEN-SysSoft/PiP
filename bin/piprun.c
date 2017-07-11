/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

/** \addtogroup piprun piprun
 *
 * \brief command to run programs using PiP
 *
 * \section synopsis SYNOPSIS
 *
 *	\c \b piprun [-n &lt;N&gt;] &lt;program&gt; ...
 *
 * \section description DESCRIPTION
 * \b Run a program under PiP. If \b -n &lt;N&gt; is specified, then
 * \b N PiP tasks are created and run.
 *
 *
 * \section environment ENVIRONMENT
 *
 * \subsection PIP_MODE PIP_MODE
 */

#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pip.h>

static void print_usage( void ) {
  fprintf( stderr, "%s [-e] [-n N] [-c C] <prog> ...\n", PROGRAM );
  exit( 1 );
}

int main( int argc, char **argv ) {
  int pipid  = 0;
  int ntasks = 1;
  int opts   = 0;
  int coreno = PIP_CPUCORE_ASIS;
  int i, k;
  int err    = 0;

  if( argc < 2 || argv[1] == NULL ) print_usage();

  for( i=1; *argv[i]=='-'; i++ ) {
    if( strcmp( argv[i], "-n" ) == 0 ) {
      if( argv[i+1] == NULL || ( ntasks = atoi( argv[++i] ) ) == 0 ) {
	print_usage();
      }
    } else if( strcmp( argv[i], "-e" ) == 0 ) {
      opts |= PIP_OPT_FORCEEXIT;
    } else if( strcmp( argv[i], "-c" ) == 0 ) {
      coreno = atoi( argv[++i] );
    } else if( strcmp( argv[i], "-b" ) == 0 ) {
      coreno = -100;
    }
  }
  opts |= PIP_OPT_PGRP;
  k = i;
  if( ( err = pip_init( &pipid, &ntasks, NULL, opts ) ) != 0 ) {
    fprintf( stderr, "pip_init()=%d\n", err );
  } else {
    int c;
    for( i=0; i<ntasks; i++ ) {
      if( coreno == -100 ) {
	c = i;
      } else {
	c = coreno;
      }
      pipid = i;
      err = pip_spawn( argv[k],
		       &argv[k],
		       NULL,
		       c,
		       &pipid,
		       NULL,
		       NULL,
		       NULL );
      if( err ) {
	int j;
	fprintf( stderr, "pip_spawn(%s)=%d\n", argv[1], err );
	for( j=0; j<i; j++ ) {
	  int status, mode, exst;
	  pip_wait( i, &status );
	  pip_get_mode( &mode );
	  if( mode & PIP_MODE_PROCESS ) {
	    if( WIFEXITED( status ) && ( exst = WEXITSTATUS( status ) ) > 0 ) {
	      fprintf( stderr, "PIPID[%d] exited with %d\n", i, exst );
	    } else if( WIFSIGNALED( status ) ) {
	      int sig = WTERMSIG( status );
	      fprintf( stderr,
		       "PIPID[%d] signaled (%s)\n",
		       i,
		       strsignal(sig) );
	    }
	  }
	}
	goto error;
      }
    }
    for( i=0; i<ntasks; i++ ) {
      int status;
      while( pip_wait( i, &status ) < 0 ) {
	if( errno == ECHILD ) break;
      }
    }
  }
 error:
  return err;
}
