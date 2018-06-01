/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#include <test.h>
#include <pip_ulp.h>

//#define DEBUG

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
# define NULPS		(4)
#else
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(250)
# endif
# define NULPS		(10)
#endif

int my_ulp_main( void ) {
  int pipid, schedid;

  TESTINT( pip_init( NULL, NULL, NULL, 0 ) );
  TESTINT( pip_get_pipid( &pipid ) );
  TESTINT( pip_get_ulp_sched( &schedid ) );

  fprintf( stderr,
#ifndef DEBUG
	   "<%d> Hello from ULP -- sched:%d\n",
#else
	   "\n<%d> Hello from ULP -- sched:%d\n\n",
#endif
	   pipid, schedid );
  return pipid;
}

int my_tsk_main( void ) {
  int pipid;

  TESTINT( pip_init( NULL, NULL, NULL, 0 ) );
  TESTINT( pip_get_pipid( &pipid ) );
  fprintf( stderr,
#ifndef DEBUG
	   "<%d> Hello from TASK\n",
#else
	   "\n<%d> Hello from TASK\n\n",
#endif
	   pipid );
  return pipid;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog_ulp, prog_tsk;
  pip_ulp_t 		*ulp;
  int			ntasks, nulps, pipid;
  int 			i, j, k;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 3)\n" );
  }
  if( ntasks >= 256 ) {
    fprintf( stderr,
	     "Too many number of PiP tasks (must be less than 256)\n" );
  }
  if( argc > 2 ) {
    nulps = atoi( argv[2] );
    if( nulps >= ntasks ) {
      fprintf( stderr,
	       "Number of ULPs (%d) must be larget than or equal to "
	       "the number of PiP tasks (%d)\n",
	       nulps, ntasks );
    }
  } else {
    nulps = NULPS;
  }

  pip_spawn_from_func( &prog_ulp, argv[0], "my_ulp_main", NULL, NULL );
  pip_spawn_from_func( &prog_tsk, argv[0], "my_tsk_main", NULL, NULL );
  TESTINT( pip_init( NULL, NULL, NULL, 0 ) );

  k = 0;
  while( k < ntasks ) {
    //fprintf( stderr, "i=%d/%d\n", i, ntasks );
    ulp = NULL;
    for( j=0; j<nulps; j++ ) {
      if( k >= ntasks - 2 ) break;
      //fprintf( stderr, "i=%d/%d  j=%d/%d\n", i, ntasks, j, NULPS );
      pipid = k++;
      TESTINT( pip_ulp_new( &prog_ulp, &pipid, ulp, &ulp ) );
    }
    pipid = k++;
    TESTINT( pip_task_spawn( &prog_tsk, PIP_CPUCORE_ASIS, &pipid, NULL, ulp ));
  }
  for( i=0; i<k; i++ ) {
    int status;
    TESTINT( pip_wait( i, &status ) );
    if( status == i ) {
      fprintf( stderr, "Succeeded (%d)\n", i );
    } else {
      fprintf( stderr, "pip_wait(%d):%d\n", i, status );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
