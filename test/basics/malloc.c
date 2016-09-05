/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <pthread.h>
#include <malloc.h>

#include <test.h>

#define NTIMES		(1000)
#define RSTATESZ	(256)

static char rstate[RSTATESZ];
//static struct random_data rbuf;

int my_initstate( int pipid ) {
  if( pipid < 0 ) pipid = 123456;
  errno = 0;
  //(void) initstate_r( (unsigned) pipid, rstate, RSTATESZ, &rbuf );
  (void) initstate( (unsigned) pipid, rstate, RSTATESZ );
  return errno;
}

int my_random( int32_t *rnump ) {
  errno = 0;
  //(void) random_r( &rbuf, rnump );
  *rnump = random();
  return errno;
}

size_t  min = 999999999;
size_t	max = 0;

int malloc_loop( int pipid ) {
  int i, j;

  fprintf( stderr, "<%d> enterring malloc_loop. this can take a while...\n",
	   pipid );
  for( i=0; i<NTIMES; i++ ) {
    int32_t sz;
    void *p;

    TESTINT( my_random( &sz ) );
    sz <<= 4;
    sz &= 0x0FFFF00;
    if( sz == 0 ) sz = 256;
    if( ( p = malloc( sz ) ) == NULL ) return ENOMEM;
    memset( p, ( pipid & 0xff ), sz );
    for( j=0; j<sz; j++ ) {
      if( ((unsigned char*)p)[j] != ( pipid & 0xff ) ) {
	fprintf( stderr, "<<<<%d>>>> malloc-check: p=%p:%d@%d\n",
		 pipid, p, sz, j );
	return -1;
      }
    }
    min = ( sz < min ) ? sz : min;
    max = ( sz > max ) ? sz : max;
    free( p );
  }
  return 0;
}

struct task_comm {
  int			go;
  pthread_barrier_t	barrier;
};

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  void *exp;
  int pipid, ntasks;
  int i;
  int err;

  TESTINT( !mallopt( M_MMAP_THRESHOLD, 1 ) );

  ntasks = NTASKS;
  tc.go = 0;
  exp    = (void*) &tc;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i%8, &pipid, NULL, NULL, NULL );
      if( err != 0 ) break;
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );

    TESTINT( my_initstate( PIP_PIPID_ROOT ) );

    tc.go = 1;
    pthread_barrier_wait( &tc.barrier );

    TESTINT( malloc_loop( PIP_PIPID_ROOT ) );

    for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
    TESTINT( pip_fin() );

  } else {
    struct task_comm 	*tcp = (struct task_comm*) exp;

    TESTINT( my_initstate( pipid ) );
    while( 1 ) {
      if( tcp->go ) break;
      pause_and_yield( 10 );
    }
    pthread_barrier_wait( &tcp->barrier );

    TESTINT( malloc_loop( pipid ) );
    fprintf( stderr,
	     "<PIPID=%d,PID=%d> Hello, I am fine (sz:%d--%d, %d times) !!\n",
	     pipid, getpid(), (int) min, (int) max, NTIMES );
  }
  return 0;
}
