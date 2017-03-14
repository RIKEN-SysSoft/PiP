
#include <xpmem_eval.h>
#include <xpmem.h>

int main( int argc, char **argv ) {
  xpmem_segid_t segid;
  xpmem_apid_t apid;
  struct xpmem_addr xpmaddr;
  void *addr;
  uint64_t tm;
  int x, err;

  if( argc < 2 ) {
    printf( "do not invoke this\n" );
    return 1;
  }
  printf( "argv[1]='%s'\n", argv[1] );

  segid = strtoull( argv[1], NULL, 16 );
  printf( "segid=%Lx\n", segid );
  tm = rdtscp();
  apid  = xpmem_get( segid, XPMEM_RDWR, XPMEM_PERMIT_MODE, NULL );
  tm = rdtscp() - tm;
  printf( "xpmem_get(): %lu\n", tm );
  if( apid == -1 ) printf( "xpmem_get(): %d\n", errno );
  xpmaddr.apid   = apid;
  xpmaddr.offset = 0;
  printf( "segid=%Lx\n", segid );
  tm = rdtscp();
  addr = xpmem_attach( xpmaddr, MMAP_SIZE, NULL );
  tm = rdtscp() - tm;
  printf( "xpmem_attach(): %lu\n", tm );

  x = touch( addr );
  print_time();
  printf( "sum=%d\n", x );
#ifndef PERF_PF
  x = touch( addr );
  print_time();
#endif

  tm = rdtscp();
  err = xpmem_detach( addr );
  tm = rdtscp() - tm;
  printf( "xpmem_detach()=%d: %lu\n", err, tm );

  tm = rdtscp();
  err = xpmem_release( apid );
  tm = rdtscp() - tm;
  printf( "xpmem_release()=%d: %lu\n", err, tm );

  return 0;
}
