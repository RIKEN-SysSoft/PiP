/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
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

#include <pip_internal.h>
#include <pip.h>

extern int pip_root_p_( void );
extern int pip_is_pthread_( void );

extern pip_root_t *pip_root;
extern pip_task_t *pip_task;

int pip_is_debug_build() {
#ifdef DEBUG
  return 1;
#else
  return 0;
#endif
}

int pip_is_initialized( void ) {
  return pip_task != NULL && pip_root != NULL;
}

int pip_isa_task( void ) {
  return pip_root != NULL && pip_task != NULL &&
    pip_root->task_root != pip_task;
}

int pip_is_threaded( int *flagp ) {
  if( pip_is_threaded_() ) {
    *flagp = 1;
  } else {
    *flagp = 0;
  }
  return 0;
}

int pip_kill_all_tasks( void ) {
  int pipid, i, err;

  err = 0;
  if( pip_is_initialized() ) {
    if( !pip_isa_root() ) {
      err = EPERM;
    } else {
      for( i=0; i<pip_root->ntasks; i++ ) {
	pipid = i;
	if( pip_check_pipid( &pipid ) == 0 ) {
	  if( pip_is_threaded_() ) {
	    pip_task_t *task = &pip_root->tasks[pipid];
	    task->status = PIP_W_EXITCODE( 0, SIGTERM );
	    (void) pip_kill( pipid, SIGQUIT );
	  } else {
	    (void) pip_kill( pipid, SIGKILL );
	  }
	}
      }
    }
  }
  return err;
}

int pip_debug_env( void ) {
  static int flag = 0;
  if( !flag ) {
    if( getenv( "PIP_NODEBUG" ) ) {
      flag = -1;
    } else {
      flag = 1;
    }
  }
  return flag > 0;
}
