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

#include <xpmem_eval.h>
#include <xpmem.h>
#include <sys/mman.h>
#include <stdint.h>

static inline long long overall_touch( void *region, int size ) {
  long long sum = 0;
  int i;

  for( i=0; i<size/STRIDE; i++ ) {
    sum += *((int*)region);
    region += STRIDE;
  }
  return sum;
}

int main( int argc, char **argv ) {
  xpmem_segid_t segid;
  xpmem_apid_t apid;
  struct xpmem_addr xpmaddr;
  void *addr;
#ifndef OVERALL
  uint64_t tm;
#else
  double tm;
  int iter;
#endif
  long long x;
  int err;

  x = 0;
  if( argc < 2 ) {
    printf( "do not invoke this\n" );
    return 1;
  }
#ifdef OVERALL
  if( argc < 3 ) {
    printf( "do not invoke this\n" );
    return 1;
  }
  iter = atoi( argv[2] );
#endif
  //printf( "argv[1]='%s'\n", argv[1] );

  segid = strtoull( argv[1], NULL, 16 );
#ifndef OVERALL
  //printf( "segid=%lx\n", (uint64_t) segid );
  tm = rdtscp();
#else
  tm = gettime();
#endif

  apid  = xpmem_get( segid, XPMEM_RDWR, XPMEM_PERMIT_MODE, NULL );

#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_get(): %lu\n", tm );
#endif
  if( apid == -1 ) printf( "xpmem_get(): %d\n", errno );
  xpmaddr.apid   = apid;
  xpmaddr.offset = 0;

#ifndef OVERALL
  //printf( "segid=%lx\n", (uint64_t) segid );
  tm = rdtscp();

  addr = xpmem_attach( xpmaddr, MMAP_SIZE, NULL );

  tm = rdtscp() - tm;
  printf( "xpmem_attach(): %lu\n", tm );
#endif

#ifdef AH
  errno = 0;
  tm = rdtscp();
  madvise( addr, MMAP_SIZE, MADV_HUGEPAGE );
  tm = rdtscp() - tm;
  printf( "madvise(%p)=%d: %lu\n", addr, errno, tm );
#endif

#ifndef OVERALL
  x = touch( addr );
  print_time();
  printf( "sum=%Ld\n", x );
#ifndef PERF_PF
  x = touch( addr );
  print_time();
#endif
#else
  {
    int k, l;
    for( k=0; k<MMAP_SIZE/(8*1024); k++ ) {
      xpmaddr.apid   = apid;
      xpmaddr.offset = 8*1024 * k;
      addr = xpmem_attach( xpmaddr, 8*1024, NULL );
      for( l=0; l<iter; l++ ) {
	x += overall_touch( addr, 8*1024 );
      }
    }
  }
#endif

#ifndef OVERALL
  tm = rdtscp();
#else
#ifndef DETACH
  //printf( "Overall %g [sec]  iter=%d   %d\n", gettime() - tm, iter, x );
  printf( "%g,  %Ld\n", gettime() - tm, x );
#endif
#endif

  err = xpmem_detach( addr );

#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_detach()=%d: %lu\n", err, tm );
#endif

#ifndef OVERALL
  tm = rdtscp();
#endif

  err = xpmem_release( apid );

#ifdef DETACH
  printf( "%g,  %Ld\n", gettime() - tm, x );
#endif

#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_release()=%d: %lu\n", err, tm );
#endif

  return 0;
}
