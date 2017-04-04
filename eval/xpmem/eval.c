
#include <xpmem_eval.h>
#include <xpmem.h>
#include <sys/mman.h>

static inline int overall_touch( void *region, int size ) {
  int sum = 0;
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
  int x, err;

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
  printf( "segid=%Lx\n", segid );
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
  printf( "segid=%Lx\n", segid );
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
  printf( "sum=%d\n", x );
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
  printf( "%g,  %d\n", gettime() - tm, x );
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
  printf( "%g,  %d\n", gettime() - tm, x );
#endif

#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_release()=%d: %lu\n", err, tm );
#endif

  return 0;
}
