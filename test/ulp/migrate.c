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

void ulp_main( void* null ) {
  struct expo *expop;
  int ulp_count = ULP_COUNT;
  pip_ulp_t *myself;
  int pipid_myself, pipid_sched;

  TESTINT( pip_init( &pipid_myself, NULL, (void**) &expop, 0 ) );
  TESTINT( !pip_is_ulp() );
  DBGF( "count:%d" , ulp_count );
  while( --ulp_count >= 0 ) {
    TESTINT( pip_get_ulp_sched( &pipid_sched ) );
    fprintf( stderr, "[ULPID:%d]  sched-task: %d  [%d]\n",
	     pipid_myself, pipid_sched, ulp_count );

    TESTINT( pip_ulp_myself( &myself ) );
    pip_spin_lock( &expop->lock );
    {
      PIP_ULP_ENQ_LAST( &expop->queue, myself );
    }
    pip_spin_unlock( &expop->lock );

    TESTINT( pip_ulp_suspend() );
  }
  DBG;
}

int main( int argc, char **argv ) {
  struct expo *expop;
  int ntasks, nulps;
  int i, j, pipid;

  if( argv[1] != NULL ) {
    ntasks = atoi( argv[1] );
  } else {
    ntasks = MTASKS;
  }
  if( ntasks < 1 ) {
    fprintf( stderr,
	     "Too small number of PiP tasks (must be latrger than 0)\n" );
    exit( 1 );
  } else if( ntasks > PIP_NTASKS_MAX ) {
    fprintf( stderr, "Number of PiP tasks (%d) is too large\n", ntasks );
    exit( 1 );
  }
  if( argv[1] != NULL && argv[2] != NULL ) {
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
    }
  } else {
    nulps = NULPS;
  }
  DBGF( "ntasks:%d  nulps:%d", ntasks, nulps );
  int nt = ntasks + nulps;

  expop = &expo;
  TESTINT( pip_init( &pipid, &nt, (void**) &expop, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t func;
    pip_spawn_program_t prog;

    pip_spawn_from_func( &func, argv[0], "ulp_main", NULL, NULL );
    pip_barrier_init( &expop->barr, ntasks );
    pip_spin_init( &expop->lock );
    PIP_ULP_INIT( &expop->queue );

    j = ntasks;
    for( i=0; i<nulps; i++ ) {
      pipid = j++;
      TESTINT( pip_ulp_new( &func, &pipid, &expop->queue, NULL ) );
    }
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
    pip_ulp_t *next;

    TESTINT( pip_import( PIP_PIPID_ROOT, (void**) &expop ) );
    pip_barrier_wait( &expop->barr );
    do {
      pip_spin_lock( &expop->lock );
      {
	//PIP_ULP_QUEUE_DESCRIBE( &expop->queue );
	if( PIP_ULP_ISEMPTY( &expop->queue) ) {
	  next = NULL;
	} else {
	  next = PIP_ULP_NEXT( &expop->queue );
	  PIP_ULP_DEQ( next );
	}
      }
      pip_spin_unlock( &expop->lock );

      if( next != NULL ) {
	int err;
	//TESTINT( pip_ulp_migrate( next, 0 ) );
	pip_spin_lock( &expop->lock );
	{
	  err = pip_ulp_migrate( next, 0 );
	  if( err ) pip_ulp_queue_describe( stderr, NULL, &expop->queue );
	  TESTINT( err );
	}
	pip_spin_unlock( &expop->lock );
	TESTINT( pip_ulp_yield_to( next ) );
      }
    } while( next != NULL );
  }
  TESTINT( pip_fin() );
  return 0;
}
