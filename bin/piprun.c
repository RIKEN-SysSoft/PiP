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
 *	\c \b piprun &lt;program&gt; ...
 *
 *
 * \section description DESCRIPTION
 * \b Run a program under PiP.
 *
 * \section environment ENVIRONMENT
 *
 * \subsection PIP_MODEL PIP_MODEL
 */

#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>
#include <pip.h>

int main( int argc, char **argv ) {
  int pipid  = 0;
  int ntasks = 1;
  int err    = 0;

  if( argc < 2 || argv[1] == NULL ) {
    fprintf( stderr, "%s <prog> ...\n", PROGRAM );
  } else if( ( err = pip_init( &pipid, &ntasks, NULL, 0 ) ) != 0 ) {
    fprintf( stderr, "pip_init()=%d\n", err );
  } else {
    pipid = PIP_PIPID_ANY;
    err = pip_spawn( argv[1],
		     &argv[1],
		     NULL,
		     PIP_CPUCORE_ASIS,
		     &pipid,
		     NULL,
		     NULL,
		     NULL );
    if( err ) {
      fprintf( stderr, "pip_spawn(%s)=%d\n", argv[1], err );
    } else {
      int status;
      while( wait( &status ) < 0 ) {
	if( errno == ECHILD ) break;
      }
    }
  }
  return err;
}
