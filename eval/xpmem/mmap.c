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

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

#include <xpmem_eval.h>
#include <xpmem.h>

#define SIZE1MB		((size_t)(1*1024*1024))

//#define USE_HUGETLB

void 	*mmapp;
size_t	size;

struct sync_st {
  size_t	size;
  volatile int	sync0;
  volatile int	sync1;
  volatile int	sync2;
};

void init_syncs( int n, struct sync_st *syncs ) {
  syncs->sync0 = n;
  syncs->sync1 = n;
  syncs->sync2 = n;
}

void wait_sync( volatile int *syncp ) {
  __sync_fetch_and_sub( syncp, 1 );
  while( *syncp > 0 );
}

void wait_sync0( struct sync_st *syncs ) {
  wait_sync( &syncs->sync0 );
}

void wait_sync1( struct sync_st *syncs ) {
  wait_sync( &syncs->sync1 );
}

void wait_sync2( struct sync_st *syncs ) {
  wait_sync( &syncs->sync2 );
}

void print_page_table_size( void ) {
  sleep( 1 );			/* we need this, otherwise /proc/meminfo */
  system( "grep PageTables /proc/meminfo" );
#ifdef USE_HUGETLB
  system( "grep HugePages  /proc/meminfo" );
#endif
}

void touch_region( struct sync_st *syncs, xpmem_segid_t segid ) {
  xpmem_apid_t apid = xpmem_get( segid, XPMEM_RDWR, XPMEM_PERMIT_MODE, NULL );
  struct xpmem_addr xpmaddr;
  void *p, *q;

  xpmaddr.apid   = apid;
  xpmaddr.offset = 0;
  p = xpmem_attach( xpmaddr, size, NULL );
#ifdef AHAH
#endif
  q = p + syncs->size;
  for( ; p<q; p+=4096 ) {
    int *ip = (int*) p;
    *ip = 123456;
  }
}

xpmem_segid_t mmap_memory( char *arg, int ntasks ) {
  if( ( size = atoi( arg ) ) == 0 ) {
    size = SIZE1MB;
  } else {
    size *= SIZE1MB;
  }
  //size *= ntasks;
  mmapp = mmap( NULL,
		size,
		PROT_READ|PROT_WRITE,
#ifdef USE_HUGETLB
		MAP_HUGETLB |
#endif
		MAP_SHARED|MAP_ANONYMOUS,
		-1,
		0 );
  TESTSC( ( mmapp == MAP_FAILED ) );
  memset( mmapp, 0, size );
  return xpmem_make( mmapp, size, XPMEM_PERMIT_MODE, (void*)0666 );
}

void fork_only( int ntasks, xpmem_segid_t segid ) {
  void *mapp;
  struct sync_st *syncp;
  pid_t pid;
  int i;

  mapp = mmap( NULL,
	       4096,
	       PROT_READ|PROT_WRITE,
	       MAP_SHARED|MAP_ANONYMOUS,
	       -1,
	       0 );
  TESTSC( ( mapp == MAP_FAILED ) );

  syncp = (struct sync_st*) mapp;
  init_syncs( ntasks+1, syncp );
  syncp->size  = size;

  for( i=0; i<ntasks; i++ ) {
    if( ( pid = fork() ) == 0 ) {
      wait_sync0( syncp );
      touch_region( syncp, segid );
      wait_sync1( syncp );
      wait_sync2( syncp );
      exit( 0 );
    }
  }
  wait_sync0( syncp );
  wait_sync1( syncp );
  print_page_table_size();
  wait_sync2( syncp );
  for( i=0; i<ntasks; i++ ) wait( NULL );
}

int main( int argc, char **argv ) {
  xpmem_segid_t segid;
  int ntasks;

  if( argc < 3 ) {
    fprintf( stderr, "mmap-XXX <ntasks> <mmap-size[KB]>\n" );
    exit( 1 );
  }

  ntasks = atoi( argv[1] );

  print_page_table_size();
  segid = mmap_memory( argv[2], ntasks );

  fork_only( ntasks, segid );

  printf( "%d tasks, %d [MiB] mmap size\n", ntasks, (int)(size/SIZE1MB) );

  return 0;
}
