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

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <pip.h>
#include <test.h>

#define NTASKS_MAX	PIP_NTASKS_MAX

double	time_start, time_spawn, time_end;

void spawn_tasks( int ntasks ) {
  char *argv[] = { "./null", NULL };
  int pipid, i;

  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  TESTINT( ( pipid != PIP_PIPID_ROOT ) );

#ifdef AH
  pip_print_maps();
  printf( "root pid: %d\n", getpid() );
#endif

  time_start = pip_gettime();
  for( i=0; i<ntasks; i++ ) {
    pipid = i;
    TESTINT( pip_spawn( argv[0],
			argv,
			NULL,
			PIP_CPUCORE_ASIS,
			&pipid,
			NULL,
			NULL,
			NULL ) );
  }
  time_spawn = pip_gettime();
  for( i=0; i<ntasks; i++ ) {
    TESTINT( pip_wait( i, NULL ) );
  }
  time_end = pip_gettime();

  TESTINT( pip_fin() );
}

int main( int argc, char **argv ) {
  int ntasks;

  if( argc < 2 ) {
    fprintf( stderr, "spawn-XXX <ntasks>\n" );
    exit( 1 );
  }
  ntasks = atoi( argv[1] );
  if( ntasks > NTASKS_MAX ) {
    fprintf( stderr, "Illegal number of tasks is specified.\n" );
    exit( 1 );
  }

  spawn_tasks( ntasks );
  printf( "%g  %g  [sec] %d tasks\n",
	  time_spawn - time_start, time_end - time_spawn, ntasks );

  return 0;
}
