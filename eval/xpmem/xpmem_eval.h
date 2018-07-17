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
 * Written by Atsushi HORI <ahori@riken.jp>, 2017
 */

#define _GNU_SOURCE

#include <eval.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sched.h>
#include <fcntl.h>
#include <stdio.h>

//#define PERF_PFBASE
//#define PERF_PF
//#define BANDWIDTH
//#define HUGETLB
//#define OVERALL
//#define DETACH

#define NTASKS_MAX	(50)
#define NDATA		(1024)
#define MMAP_SIZE	((size_t)(2*1024*1024))

void corebind( int c ) {
  cpu_set_t cpuset;

  CPU_ZERO( &cpuset );
  CPU_SET( c, &cpuset );
  if( sched_setaffinity( 0, sizeof(cpuset), &cpuset ) != 0 ) {
    fprintf( stderr, "sched_setaffinity(%d): %d\n", c, errno );
    exit( 1 );
  }
}

static inline int create_region( void **vaddrp ) {
  void *vaddr;
  int fd, i, *data;

  fd = -1;
  if( ( fd = open( "/dev/zero", O_RDWR ) ) < 0 ) {
    printf( "open() failed\n" );
    return errno ;
  }
  if( ( vaddr = mmap( NULL,
		      MMAP_SIZE,
		      PROT_READ|PROT_WRITE,
#ifdef HUGETLB
		      MAP_HUGETLB |
#endif
		      MAP_PRIVATE | MAP_ANON,
		      fd,
		      0 ) ) == MAP_FAILED ) {
    printf( "mmap() failed\n" );
    return errno;
  }
#ifndef OVERALL
  //printf( "vaddr=%p\n", vaddr );
#endif

  *vaddrp = vaddr;
  data    = (int*) vaddr;
  close( fd );
  for( i=0; i<MMAP_SIZE/sizeof(int); i++ ) {
    data[i] = i / 128;
  }
#ifdef HUGETLB
  system( "grep -i huge /proc/meminfo" );
#endif
  return 0;
}

#ifdef PERF_PFBASE
#ifndef PERF_PF
#define PERF_PF
#endif
#endif

#define CACHEBLK	(64)
#define PAGESZ		(4096)

#ifdef PERF_PF
#define STRIDE		PAGESZ
#else
#define STRIDE		CACHEBLK
#endif

#define NITERS		(MMAP_SIZE/STRIDE)

#ifndef BANDWIDTH
#ifndef OVERALL
unsigned int xptime[NDATA];
#endif
#endif

static inline long long touch( void *region ) {
  long long sum = 0;
#ifndef PERF_PFBASE
  int i;

#ifdef BANDWIDTH
  double dtm = gettime();
#endif

  for( i=0; i<NITERS; i++ ) {
#ifndef BANDWIDTH
#ifndef OVERALL
    uint64_t delta = rdtscp();
#endif
#endif
    sum += *((int*)region);
    region += STRIDE;
#ifndef BANDWIDTH
#ifndef OVERALL
    if( i < NDATA ) xptime[i] = rdtscp() - delta;
#endif
#endif
  }

#ifndef OVERALL
#ifdef BANDWIDTH
  dtm = gettime() - dtm;
  printf( "Time to sum: %g\n", dtm );
#endif
#endif
#endif
  return sum;
}

static inline void print_time( void ) {
#ifndef BANDWIDTH
#ifndef PERF_PF
#ifndef OVERALL
  int i;
  for( i=0; i<NDATA; i++ ) {
    printf( "%u\n", xptime[i] );
  }
#endif
#endif
#endif
}
