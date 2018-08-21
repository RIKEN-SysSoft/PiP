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

#define PIP_INTERNAL_FUNCS

#define NITERS	(100*1000)
//#define NITERS	(10)

//#define DEBUG

#include <test.h>

pip_locked_queue_t	queue;

int main( int argc, char **argv ) {
  pip_locked_queue_t	*qp = &queue;
  int i, pipid;
  int ntasks = 2;

  set_sigsegv_watcher();

  TESTINT( pip_init( &pipid, &ntasks, (void**) &qp, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    pip_spawn_program_t prog;

    pip_spawn_from_main( &prog, argv[0], argv, NULL );
    TESTINT( pip_locked_queue_init( qp ) );

    pipid = 0;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL ) );
    pipid = 1;
    TESTINT( pip_task_spawn( &prog, PIP_CPUCORE_ASIS, &pipid, NULL ) );

    TESTINT( pip_wait( 0, NULL ) );
    TESTINT( pip_wait( 1, NULL ) );
  } else {
    double tm = -pip_gettime();
    for( i=0; i<NITERS; i++ ) {
      if( pipid == 0 ) {
	TESTINT( pip_sleep_and_enqueue( qp, 0 ) );
      } else {
	while( 1 ) {
	  int err = pip_dequeue_and_wakeup( qp );
	  if( err != ENOENT ) {
	    if( err != 0 ) {
	      fprintf( stderr, "pip_dequeue_and_wakeup():%d\n", err );
	    }
	    break;
	  }
	  pip_pause();
	}
      }
    }
    tm += pip_gettime();
    fprintf( stderr, "[%d] %g [S*%d]  %g [usec]\n",
	     pipid, tm, NITERS, tm / (double) NITERS * 1e6 );
  }
  TESTINT( pip_fin() );
  return 0;
}
