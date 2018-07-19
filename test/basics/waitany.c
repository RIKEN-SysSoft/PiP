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

//#define DEBUG
#include <test.h>
#include <time.h>

pip_barrier_t barr, *barrp;

void my_sleep( int n ) {
  struct timespec tm, tr;
  tm.tv_sec = 0;
  tm.tv_nsec = n * 1000 * 1000;
#ifdef DEBUG
  tm.tv_sec  = 1 * n;
  tm.tv_nsec = 0;
#endif
  (void) nanosleep( &tm, &tr );
}

int main( int argc, char **argv ) {
  int pipid = 999;
  int ntasks = 0;
  int extval;
  int flag, i;

  if( argc   > 1 ) ntasks = atoi( argv[1] );
  if( ntasks < 1 ) ntasks = NTASKS;
  ntasks = ( ntasks > 256 ) ? 256 : ntasks;

  barrp = &barr;
  pip_barrier_init( barrp, ntasks+1 );

  TESTINT( pip_init( &pipid, &ntasks, (void**) &barrp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      TESTINT( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			  NULL, NULL, NULL ) );
    }
    pip_barrier_wait( barrp );
    my_sleep( ntasks / 2 );
    flag = 0;
    for( i=0; i<ntasks; i++ ) {
      TESTINT( pip_wait_any( &pipid, &extval ) );
      if( pipid != extval ) {
	flag = 1;
	fprintf( stderr, "NGNGNG [ROOT] PIPID[%d] %d\n", pipid, extval );
      } else {
#ifdef DEBUG
	fprintf( stderr, "OKOKOK [ROOT] PIPID[%d] %d\n", pipid, extval );
#endif
      }
    }
    if( !flag ) {
      fprintf( stderr, "SUCCEEDED\n" );
    } else {
      fprintf( stderr, "FAILED\n" );
    }
    TESTINT( pip_fin() );

  } else {
    pip_barrier_wait( barrp );
    if( pipid > ntasks/2 ) my_sleep( ntasks - pipid );
    //fprintf( stderr, "[%d] Hello, I am fine !!\n", pipid );
    pip_exit( pipid );
    /* never reach here */
  }
  return 0;
}
