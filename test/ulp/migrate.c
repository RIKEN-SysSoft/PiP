/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define PIP_INTERNAL_FUNCS

#define DEBUG

#include <test.h>
#include <pip_ulp.h>

#ifndef DEBUG
#define ULP_COUNT 	(100);
#else
#define ULP_COUNT 	(10);
#endif

#ifdef DEBUG
# define MTASKS	(10)
#else
# define MTASKS	(NTASKS/2)
#endif

#define NULPS		(NTASKS/2)

struct expo {
  pip_barrier_t		barr;
  pip_spinlock_t	lock;
  pip_ulp_t		queue;
} expo;
struct expo *expop;

int ulp_count = ULP_COUNT;

void ulp_main( void* null ) {
  pip_ulp_t *myself;
  int pipid_myself, pipid_sched;

  TESTINT( pip_init( &pipid_myself, NULL, NULL, 0 ) );
  DBG;
  while( 1 ) {
    TESTINT( pip_get_ulp_sched( &pipid_sched ) );
    if( --ulp_count <= 0 ) break;
    fprintf( stderr, "[PIPID:%d] sched: %d  [%d]\n",
	     pipid_myself, pipid_sched, ulp_count );
    TESTINT( pip_ulp_myself( &myself ) );

    pip_spin_lock( &expop->lock );
    {
      PIP_ULP_ENQ_LAST( &expop->queue, myself );
    }
    pip_spin_unlock( &expop->lock );

    TESTINT( pip_ulp_suspend() );
  }
}

int main( int argc, char **argv ) {
  int ntasks, nulps;
  int i, j, pipid;

  if( argc > 1 ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = MTASKS;
  }
  if( ntasks < 2 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 1)\n" );
    exit( 1 );
  } else if( ntasks > PIP_NTASKS_MAX ) {
    fprintf( stderr, "Number of PiP tasks (%d) is too large\n", ntasks );
    exit( 1 );
  }
  if( argc > 2 ) {
    nulps = atoi( argv[2] );
    if( nulps == 0 ) {
      fprintf( stderr, "Number of ULPs must be larget than zero\n" );
      exit( 1 );
    } else if( nulps > PIP_NTASKS_MAX ) {
      fprintf( stderr,
	       "Number of PiP tasks (%d) and ULPs (%d) are too large\n",
	       ntasks,
	       nulps );
      exit( 1 );
    } else if ( nulps < ntasks ) {
      fprintf( stderr,
	       "Number of PiP tasks (%d) must be larger than "
	       "or equal to the number of ULPs (%d) \n",
	       ntasks,
	       nulps );
      exit( 1 );
    }
  } else {
    nulps = NULPS;
  }

  TESTINT( pip_init( &pipid, NULL, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t func;
    pip_spawn_program_t prog;

    pip_spawn_from_func( &func, argv[0], "ulp_main", NULL, NULL );
    pip_barrier_init( &expo.barr, ntasks );
    pip_spin_init( &expo.lock );
    PIP_ULP_INIT( &expo.queue );

    j = MTASKS;
    for( i=0; i<nulps; i++ ) {
      pipid = j++;
      TESTINT( pip_ulp_new( &func, &pipid, &expo.queue, NULL ) );
    }
    //PIP_ULP_ENQ_FIRST( ulp, &expo.queue );
    PIP_ULP_QUEUE_DESCRIBE( &expo.queue );
    TESTINT( pip_export( (void*) &expo ) );
    DBGF( "expop: %p", &expo );;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    j = 0;
    for( i=0; i<ntasks; i++ ) {
      pipid = j++;
      TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL, NULL ) );
    }
    for( i=0; i<ntasks+nulps; i++ ) {
      TESTINT( pip_wait( i, NULL ) );
    }
  } else {
    TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
    pip_barrier_wait( &expop->barr );
    DBGF( "expop: %p", expop );;

    while( 1 ) {
      pip_ulp_t *next;

      DBG;
      pip_spin_lock( &expop->lock );
      {
	PIP_ULP_QUEUE_DESCRIBE( &expop->queue );
	if( PIP_ULP_ISEMPTY( &expop->queue) ) break;
	next = PIP_ULP_NEXT( &expop->queue );
	PIP_ULP_DEQ( &expop->queue );
      }
      pip_spin_unlock( &expop->lock );

      TESTINT( pip_ulp_resume( next, 0 ) );
    }
    DBG;
    pip_spin_unlock( &expop->lock );
  }
  TESTINT( pip_fin() );
  return 0;
}
