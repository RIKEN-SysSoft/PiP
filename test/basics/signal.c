/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG
#include <test.h>

int send_wait_signal( int pipid, int ssig ) {
  sigset_t sigset;
  int wsig;
  int err;

  DBGF( "Root sends %s", signal_name( ssig ) );
  if( ( err = pip_kill( pipid, ssig ) ) == 0 ) {
    (void) sigfillset( &sigset );
    if( sigwait( &sigset, &wsig ) != 0 ) {
      err = errno;
    } else if( wsig != ssig ) {
      fprintf( stderr, "send_wait: SIGANL %d!=%d\n", ssig, wsig );
      err = ENXIO;
    } else {
      DBGF( "Root received %s", signal_name( wsig ) );
    }
  }
  RETURN( err );
}

int wait_send_signal( void ) {
  sigset_t sigset;
  int sig;
  int err;

  (void) sigfillset( &sigset );
  if( sigwait( &sigset, &sig ) != 0 ) {
    err = errno;
  } else if( ( err = pip_kill( PIP_PIPID_ROOT, sig ) ) != 0 ) {
    err = errno;
  }
  RETURN( err );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks;
  volatile int flag = 0;
  void *exp;

  ntasks = 1;
  exp = (void*) &flag;
  TESTINT( pip_init( &pipid, &ntasks, &exp, PIP_MODEL_PROCESS ) );
  if( pipid == PIP_PIPID_ROOT ) {

    watch_anysignal();

    pipid = 0;
    TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid ) );

    while( flag == 0 ) pause_and_yield( 0 );

    TESTINT( pip_kill( pipid, SIGHUP  ) );
    TESTINT( pip_kill( pipid, SIGINT  ) );
    TESTINT( pip_kill( pipid, SIGQUIT ) );
    TESTINT( pip_kill( pipid, SIGILL  ) );
    TESTINT( pip_kill( pipid, SIGABRT ) );
    TESTINT( pip_kill( pipid, SIGFPE  ) );
    TESTINT( pip_kill( pipid, SIGSEGV ) );
    TESTINT( pip_kill( pipid, SIGPIPE ) );
    TESTINT( pip_kill( pipid, SIGALRM ) );
    TESTINT( pip_kill( pipid, SIGTERM ) );
    TESTINT( pip_kill( pipid, SIGUSR1 ) );
    TESTINT( pip_kill( pipid, SIGUSR2 ) );
    TESTINT( pip_kill( pipid, SIGCHLD ) );
    TESTINT( pip_kill( pipid, SIGTSTP ) );
    TESTINT( pip_kill( pipid, SIGTTIN ) );
    TESTINT( pip_kill( pipid, SIGTTOU ) );
    //TESTINT( pip_kill( pipid, SIGSTOP ) );
    TESTINT( pip_kill( pipid, SIGCONT ) );
    //TESTINT( pip_kill( pipid, SIGKILL ) );

    flag = 1;
    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_fin() );

  } else {
    echo_anysignal();

    *(int*)exp = 1;

    while( *(int*)exp == 1 ) pause_and_yield( 0 );
  }
  return 0;
}
