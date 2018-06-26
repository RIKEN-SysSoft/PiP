/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <signal.h>

//#define DEBUG
#include <test.h>
#include <time.h>

pip_barrier_t barr, *barrp;

void my_sleep( int n ) {
  struct timespec tm, tr;
  tm.tv_sec = 0;
  tm.tv_nsec = n * 1000 * 1000;
#ifdef DEBUG
  tm.tv_sec  = 1 * n;
  tm.tv_nsec = 0;
#endif
  (void) nanosleep( &tm, &tr );
}

int count_sigchld = 0;;

void sigchld_handler( int sig ) {
#ifdef DEBUG
  fprintf( stderr, "SIGCHLD\n" );
#endif
  count_sigchld ++;
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks = 0;
  int i;

  if( argc   > 1 ) ntasks = atoi( argv[1] );
  if( ntasks < 1 ) ntasks = NTASKS;
  ntasks = ( ntasks > 256 ) ? 256 : ntasks;

  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    struct sigaction 	sigact;

    memset( &sigact, 0, sizeof( sigact ) );
    TESTINT( sigemptyset( &sigact.sa_mask ) );
    TESTINT( sigaddset( &sigact.sa_mask, SIGCHLD ) );
    //sigact.sa_flags = SA_SIGINFO;
    sigact.sa_sigaction = (void(*)()) sigchld_handler;
    TESTINT( sigaction( SIGCHLD, &sigact, NULL ) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			  NULL, NULL, NULL ) );
      TESTINT( pip_wait_any( NULL, NULL ) );
    }
    if( count_sigchld == ntasks ) {
      fprintf( stderr, "SUCCEEDED\n" );
    } else {
      fprintf( stderr, "FAILED (%d!=%d)\n", count_sigchld, ntasks );
    }
    TESTINT( pip_fin() );
  } else {
    pip_exit( pipid );
    /* never reach here */
  }
  return 0;
}
