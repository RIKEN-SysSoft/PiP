/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the FreeBSD Project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

//#define DEBUG

#define PIP_EXPERIMENTAL
#include <test.h>
#include <pip_internal.h>
#include <pthread.h>
#include <malloc.h>

#define NTIMES		(100)
#define RSTATESZ	(256)

static char rstate[RSTATESZ];
//static struct random_data rbuf;

struct task_comm {
  int			go;
  pthread_barrier_t	barrier;
  struct {
    pip_spinlock_t	lock;
    int			pipid;
    size_t		sz;
    void 		*chunk;
  } each[NTASKS];
};

extern void *pip_malloc( size_t );
extern void pip_free( void* );

#define PIP_MALLOC	pip_malloc
#define PIP_FREE	pip_free
/***
#define PIP_MALLOC	malloc
#define PIP_FREE	free
***/

int my_initstate( int pipid ) {
  if( pipid < 0 ) pipid = 654321;
  errno = 0;
  (void) initstate( (unsigned) pipid, rstate, RSTATESZ );
  return errno;
}

int my_random( int32_t *rnump ) {
  errno = 0;
  *rnump = random();
  return errno;
}

size_t  min = 999999999;
size_t	max = 0;
int   	ntasks;

void check_and_free( int pipid, void *p, size_t sz ) {
  int i;
  for( i=0; i<sz; i++ ) {
    if( ((unsigned char*)p)[i] != ( pipid & 0xff ) ) {
      fprintf( stderr, "<<<<%d>>>> malloc-check: p=%p:%d@%d\n",
	       pipid, p, (int) sz, i );
    }
  }
  DBGF( "free(%p)@%d", p, pipid );
  PIP_FREE( p );
}

int malloc2_loop( int pipid, struct task_comm *tcp ) {
  int i;

  fprintf( stderr, "<%d> entering malloc2_loop. this can take a while...\n",
	   pipid );
  for( i=0; i<NTIMES; i++ ) {
    int32_t nd, sz;
    void *p;

    while( 1 ) {
      my_random( &nd );
      nd %= ntasks;
      if( pip_spin_trylock( &tcp->each[nd].lock ) ) break;
      pip_pause();
    }
    { /* begin of locked region */

      if( tcp->each[nd].chunk != NULL ) {
	check_and_free( tcp->each[nd].pipid,
			tcp->each[nd].chunk,
			tcp->each[nd].sz );
      }
      my_random( &sz );
      sz <<= 4;
      sz &= 0x0FFFF0;
      if( sz == 0 ) sz = 256;

      p = PIP_MALLOC( sz );
      tcp->each[nd].pipid = pipid;
      tcp->each[nd].chunk = p;
      tcp->each[nd].sz    = sz;
      if( p != NULL ) memset( p, ( pipid & 0xff ), sz );

      DBGF( "malloc(%p)@%d", p, pipid );

      min = ( sz < min ) ? sz : min;
      max = ( sz > max ) ? sz : max;

    } /* end of locked region */
    pip_spin_unlock( &tcp->each[nd].lock );

  }
  return 0;
}

int main( int argc, char **argv ) {
  struct task_comm 	tc;
  struct task_comm 	*tcp;
  void *exp;
  int pipid;
  int i;
  int err;

  ntasks = NTASKS;
  tc.go = 0;
  for( i=0; i<NTASKS; i++ ) {
    pip_spin_init( &tc.each[i].lock );
    tc.each[i].chunk = NULL;
  }
  exp = (void*) &tc;

  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  tcp = (struct task_comm*) exp;
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, NTASKS, strerror( err ) );
	break;
      }
      if( i != pipid ) {
	fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
      }
    }
    ntasks = i;
    TESTINT( pthread_barrier_init( &tc.barrier, NULL, ntasks+1 ) );

    TESTINT( my_initstate( PIP_PIPID_ROOT ) );

    tc.go = 1;
    pthread_barrier_wait( &tc.barrier );

    TESTINT( malloc2_loop( PIP_PIPID_ROOT, tcp ) );
    pthread_barrier_wait( &tc.barrier );

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

    TESTINT( malloc2_loop( pipid, tcp ) );
    pthread_barrier_wait( &tcp->barrier );
    fprintf( stderr,
	     "<PIPID=%d,PID=%d> Hello, I am fine (sz:%d--%d, %d times) !!\n",
	     pipid, getpid(), (int) min, (int) max, NTIMES );
  }
  return 0;
}
