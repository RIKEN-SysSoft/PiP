/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

//#define DEBUG

#include <test.h>
#include <pip_ulp.h>

#ifdef DEBUG
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(10)
# endif
#else
# ifdef NTASKS
#  undef NTASKS
#  define NTASKS	(250)	/* must be less tahn 256 (0xff) */
# endif
#endif

#ifndef DEBUG
#define NULPS		(NTASKS-10)
#else
#define NULPS		(NTASKS-5)
#endif

#define BIAS		(10000)

struct expo {
  volatile int		c;
  pip_ulp_barrier_t 	barr;
} expo;

int main( int argc, char **argv ) {
  struct expo *expop;
  int ntasks = 0;
  int i, pipid, nulps;

  if( argc   > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = NTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
  }
  nulps = ntasks - 1;

  pip_ulp_barrier_init( &expo.barr, ntasks );
  expo.c = BIAS;
  expop  = &expo;
  TESTINT( pip_init( &pipid, NULL, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;
    pip_ulp_t *ulp = NULL;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    for( i=0; i<nulps; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_new( &prog, &pipid, ulp, &ulp ) );
    }
    pipid = i;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, ulp ));
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    if( pip_is_ulp() ) {

      /* Disturbances */
      int nyields = rand() % nulps;
      for( i=0; i<nyields; i++ ) TESTINT( pip_ulp_yield() );

      for( i=0; i<nulps; i++ ) {
	TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
	if( i == pipid ) {
	  if( (expop->c)++ == ( pipid + BIAS ) ) {
	    fprintf( stderr, "<%d> Hello ULP !!\n", pipid );
	  } else {
	    fprintf( stderr, "<%d> Bad ULP !!\n", pipid );
	  }
	}
      }
    } else {
      for( i=0; i<nulps; i++ ) {
	TESTINT( pip_ulp_barrier_wait( &expop->barr ) );
      }
      if( pipid == ( ntasks - 1 ) && expop->c == ( pipid + BIAS ) ) {
	fprintf( stderr, "<%d> Hello TASK !!\n", pipid );
      } else {
	fprintf( stderr, "<%d> Bad TASK !!\n", pipid );
      }
    }
  }
  TESTINT( pip_fin() );
  return 0;
}
