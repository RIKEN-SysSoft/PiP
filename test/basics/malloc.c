/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the PiP project.$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#include <test.h>
#include <pthread.h>
#include <malloc.h>

#define NTIMES		(100)
#define RSTATESZ	(256)

static char rstate[RSTATESZ];
//static struct random_data rbuf;

int my_initstate( int pipid ) {
  if( pipid < 0 ) pipid = 123456;
  errno = 0;
  (void) initstate( (unsigned) pipid, rstate, RSTATESZ );
  return errno;
}

int my_random( int32_t *rnump ) {
  errno = 0;
  //(void) random_r( &rbuf, rnump );
  *rnump = random();
  return errno;
}

int	ntasks = 0;
size_t  min = 999999999;
size_t	max = 0;

int malloc_loop( int pipid ) {
  unsigned long mem = get_total_memory(); /* KB */
  unsigned long mask;
  int i, j;

  mem *= 1024;
  if( pipid == PIP_PIPID_ROOT && isatty( 1 ) ) {
    printf( "mem:0x%lx  %ld MiB \n", mem, mem/(1024*1024) );
  }
  mem /= ( ntasks + 1 ) * 2;
  if( pipid == PIP_PIPID_ROOT && isatty( 1 ) ) {
    printf( "mem:0x%lx  %ld MiB\n", mem, mem/(1024*1024) );
  }

  mask = 1024 * 1024;
  while( mask < mem ) mask *= 2;
  mask -= 1;

  if( pipid == PIP_PIPID_ROOT && isatty( 1 ) ) {
    printf( "mask:0x%lx  %ld MiB\n", mask, mask/(1024*1024) );
  }

  fprintf( stderr, "<%d> enterring malloc_loop. this can take a while...\n",
	   pipid );
  for( i=0; i<NTIMES; i++ ) {
    int32_t sz;
    void *p;

    TESTINT( my_random( &sz ) );
    sz <<= 6;
    sz &= mask;
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
  int pipid;
  int i;
  int err;

  if( argc    > 1 ) ntasks = atoi( argv[1] );
  if( ntasks == 0 ) ntasks = NTASKS;

  tc.go = 0;
  exp   = (void*) &tc;
  TESTINT( pip_init( &pipid, &ntasks, &exp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<NTASKS; i++ ) {
      pipid = i;
      err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
		       &pipid, NULL, NULL, NULL );
      if( err != 0 ) {
	fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		 i, ntasks, strerror( err ) );
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
    if( isatty( 1 ) ) {
      fprintf( stderr,
	       "<PIPID=%d> Hello, I am fine (sz:%d--%d[KiB], %d times) !!\n",
	       pipid, (int) min/1024, (int) max/1024, NTIMES );
    }
  }
  return 0;
}
