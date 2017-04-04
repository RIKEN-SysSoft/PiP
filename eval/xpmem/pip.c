/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <xpmem_eval.h>
#include <pip.h>
#include <xpmem.h>

int main( int argc, char **argv ) {
  int pipid, ntasks;
  xpmem_segid_t segid;
  char *nargv[4], segidstr[32];
  void *vaddr;
  uint64_t tm;

#ifdef OVERALL
  if( argc < 2 ) {
    printf( "no iterration\n" );
    exit( 1 );
  }
#endif

  pipid  = 0;
  ntasks = 10;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );

  TESTINT( create_region( &vaddr ) );
  tm = rdtscp();
  segid = xpmem_make( vaddr, MMAP_SIZE, XPMEM_PERMIT_MODE, (void*)0666 );
#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_make(): %lu\n", tm );
#endif
  if( segid == -1 ) printf( "xpmem_make(): %d\n", errno );
  sprintf( segidstr, "%lx", vaddr );

  nargv[0] = "./eval-pip";
  nargv[1] = segidstr;
  nargv[2] = argv[1];
  nargv[3] = NULL;

  pipid = 0;
  TESTINT( pip_spawn( nargv[0], nargv, NULL, 1, &pipid, NULL, NULL, NULL ) );
  pip_wait( 0, NULL );

  TESTINT( pip_fin() );
  return 0;
}
