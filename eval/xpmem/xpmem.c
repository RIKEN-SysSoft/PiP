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

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

int main( int argc, char **argv ) {
  extern char **environ;
  xpmem_segid_t segid;
  pid_t pid;
  char *nargv[4], segidstr[32];
  void *vaddr;
  uint64_t tm;

#ifdef OVERALL
  if( argc < 2 ) {
    printf( "no iteration\n" );
    exit( 1 );
  }
#else
  int ver = xpmem_version();
  //printf( "XPMEM: %d 0x%x\n", ver, ver );
#endif
  TESTINT( create_region( &vaddr ) );
  tm = rdtscp();
  segid = xpmem_make( vaddr, MMAP_SIZE, XPMEM_PERMIT_MODE, (void*)0666 );
#ifndef OVERALL
  tm = rdtscp() - tm;
  printf( "xpmem_make(): %lu\n", tm );
#endif
  if( segid == -1 ) {
    printf( "xpmem_make(): errno=%d\n", errno );
  } else {
    if( ( pid = fork() ) == 0 ) {
      corebind( 1 );
      sprintf( segidstr, "%lx", segid );
      //printf( "fork(%s)\n", segidstr );

      nargv[0] = "./eval-xpmem";
      nargv[1] = segidstr;
      nargv[2] = argv[1];
      nargv[3] = NULL;

      execve( nargv[0], nargv, environ );
      printf( "????\n" );
    }
    wait( NULL );
  }
  return 0;
}
