/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 2.0.0$
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
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <test.h>

#define NTIMES		(100)

struct task_comm {
  pthread_mutex_t	mutex[2];
  int			count;
};

int main( int argc, char **argv ) {
  struct task_comm	tc;
  struct task_comm 	*tcp = &tc;
  void 		*exp;
  int 		pipid  = 999;
  int 		ntasks = 1, i;

  exp = (void*) tcp;
  CHECK( pip_init( &pipid, &ntasks, &exp, 0 ),         RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pthread_mutex_init( &tcp->mutex[0], NULL ), RV, return(EXIT_FAIL) );
    CHECK( pthread_mutex_init( &tcp->mutex[1], NULL ), RV, return(EXIT_FAIL) );
    CHECK( pthread_mutex_lock( &tcp->mutex[0] ),       RV, return(EXIT_FAIL) );
    CHECK( pthread_mutex_lock( &tcp->mutex[1] ),       RV, return(EXIT_FAIL) );
    tc.count = 0;

    pipid = 0;
    CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		      NULL, NULL, NULL ),
	   RV,
	   return(EXIT_FAIL) );

    for( i=0; i<NTIMES; i++ ) {
      CHECK( pthread_mutex_lock(   &tc.mutex[0] ), RV, return(EXIT_FAIL) );
      CHECK( pthread_mutex_unlock( &tc.mutex[1] ), RV, return(EXIT_FAIL) );
    }
    CHECK( pthread_mutex_unlock( &tc.mutex[0] ),   RV, return(EXIT_FAIL) );

    CHECK( pip_wait( 0, NULL ), RV, return(EXIT_FAIL) );

  } else {
    tcp = (struct task_comm*) exp;

    for( i=0; i<NTIMES; i++ ) {
      CHECK( pthread_mutex_unlock( &tcp->mutex[0] ), RV, return(EXIT_FAIL) );
      CHECK( pthread_mutex_lock(   &tcp->mutex[1] ), RV, return(EXIT_FAIL) );
    }
    CHECK( pthread_mutex_unlock( &tcp->mutex[1] ),   RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
