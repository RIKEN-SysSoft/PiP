/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define NTHREADS	(100)

#define DEBUG

#define PIP_INTERNAL_FUNCS
#include <test.h>

pthread_barrier_t	barrier;

void *thread( void *argp ) {
  // TESTINT( pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, NULL ) );
  // the PTHREAD_CANCEL_DISABLE helps nothing
  // pthread_exit( NULL );  /* pthread_exit() does SOMETHING WRONG !!!! */
  // pthread_exit() cancels myself, indeed
  return NULL;
}

int main( int argc, char **argv ) {
  pthread_barrier_t *barrp;
  int pipid    = 999;
  int ntasks   = NTASKS;
  int nthreads = NTHREADS;
  int i;
  int err;

  DBG;
  if( !pip_isa_piptask() ) {
    //TESTINT( pthread_barrier_init( &barrier, NULL, ntasks+1 ) );
    barrp = &barrier;
    if( argc      > 1 ) ntasks = atoi( argv[1] );
    if( ntasks   <= 0 ||
	ntasks   >  NTASKS ) ntasks = NTASKS;
  } else {
    if( argc      > 2 ) nthreads = atoi( argv[2] );
    if( nthreads <= 0 ||
	nthreads >  NTHREADS ) nthreads = NTHREADS;
  }

  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%8, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
#define SERIAL
#ifndef SERIAL
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
#endif
      TESTINT( pip_wait( i, NULL ) );
    }

  } else {
    pthread_t threads[NTHREADS];
    for( i=0; i<nthreads; i++ ) {
      TESTINT( pthread_create( &threads[i],NULL, thread, NULL ) );
#ifndef SERIAL
    }
    DBG;
    for( i=0; i<nthreads; i++ ) {
#endif
      //while( pthread_tryjoin_np( threads[i], NULL ) != 0 );
      TESTINT( pthread_join( threads[i], NULL ) );
      printf( "pthread_join\n" );
    }
  }
  TESTINT( pip_fin() );
  DBG;
  return 0;
}
