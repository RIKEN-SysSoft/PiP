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
  int pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  return pipid;
}

int my_tsk_main( void ) {
  int pipid;

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  return pipid;
}

int main( int argc, char **argv ) {
  pip_spawn_program_t	prog_ulp, prog_tsk;
  pip_ulp_t 		ulps;
  int			ntasks, nulps, pipid;
  int 			i, j, k;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  }
  if( ntasks >= 256 ) {
    fprintf( stderr,
	     "Too many number of PiP tasks (must be less than 256)\n" );
    exit( 1 );
  }
  if( argc > 2 ) {
    nulps = atoi( argv[2] );
    if( nulps >= ntasks ) {
      fprintf( stderr,
	       "Number of ULPs (%d) must be less than "
	       "the number of PiP tasks (%d)\n",
	       nulps, ntasks );
      exit( 1 );
    }
  } else {
    nulps = NULPS;
  }

  pip_spawn_from_func( &prog_ulp, argv[0], "my_ulp_main", NULL, NULL );
  pip_spawn_from_func( &prog_tsk, argv[0], "my_tsk_main", NULL, NULL );
  TESTINT( pip_init( NULL, NULL, NULL, 0 ) );

  k = 0;
  while( k < ntasks ) {
    PIP_ULP_INIT( &ulps );
    //fprintf( stderr, "i=%d/%d\n", i, ntasks );
    for( j=0; j<nulps; j++ ) {
      if( k >= ntasks - 2 ) break;
      //fprintf( stderr, "i=%d/%d  j=%d/%d\n", i, ntasks, j, NULPS );
      pipid = k++;
      TESTINT( pip_ulp_new( &prog_ulp, &pipid, &ulps, NULL ) );
    }
    pipid = k++;
    TESTINT( pip_task_spawn( &prog_tsk,
			     PIP_CPUCORE_ASIS,
			     &pipid,
			     NULL,
			     &ulps ));
  }
  for( i=0; i<k; i++ ) {
    int status;
    TESTINT( pip_wait_any( &pipid, &status ) );
    if( status == pipid && status == i ) {
      fprintf( stderr, "[%d] Succeeded (%d)\n", pipid, status );
    } else {
      fprintf( stderr, "[%d] pip_wait_any(%d):%d -- FAILED\n",
	       pipid, i, status );
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
