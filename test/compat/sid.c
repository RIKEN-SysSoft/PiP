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

#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <pip.h>

#define NTASKS	(10)

int main( int argc, char **argv ) {
  pid_t sid_old, sid_new;
  int  i, ntasks, pipid;

  ntasks = NTASKS;

  sid_old = getsid( getpid() );

  pip_init( &pipid, &ntasks, NULL, PIP_MODE_PROCESS );
  if( pipid == PIP_PIPID_ROOT ) {
    if( ( sid_new = setsid() ) < 0 ) {
      printf( "ROOT: setsid(): %d\n", errno );
    }
    printf( "ROOT[%d]: sid %d -> %d\n", getpid(), sid_old, sid_new );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      pip_spawn( argv[0], argv, NULL, i, &pipid, NULL, NULL, NULL );
    }
    for( i=0; i<ntasks; i++ ) wait( NULL );
    sid_new = getsid( getpid() );
    printf( "ROOT[%d]: sid %d\n", getpid(), sid_new );
    printf( "all done\n" );

  } else {	/* PIP child task */
    if( ( sid_new = setsid() ) < 0 ) {
      printf( "CHILD: setsid(): %d\n", errno );
    }
    printf( "CHILD[%d]: sid %d -> %d\n", getpid(), sid_old, sid_new );
  }
  pip_fin();
  return 0;
}
