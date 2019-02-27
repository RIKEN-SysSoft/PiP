/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.1$
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

#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

int pipid;

int open_file( void ) {
#define TEMPDIR		"/tmp/basic-XXXXXX"
  char *template = strdup( TEMPDIR );
  int nfd, fd;

  if( template == NULL ) return -1;
  fd = mkstemp( template );
  DBGF( "[PIPID=%d] FILE=%s", pipid, template );
  (void) unlink( template );
  free( template );

  if( pipid == PIP_PIPID_ROOT ) {
    nfd = dup2( fd, 90 );
  } else {
    nfd = dup2( fd, pipid + 100 );
  }
  (void) close( fd );
  return nfd;
}

int flag_NG = 0;

void check_fd( int fd, int n, int flag ) {
  char *node_str( int n ) {
    static char ndstr[64];
    if( n == PIP_PIPID_ROOT ) return "ROOT";
    sprintf( ndstr, "%d", n );
    return ndstr;
  }
  struct stat stbuf;
  char *self = node_str( pipid );
  char *node = node_str( n );

  if( fstat( fd, &stbuf ) != 0 ) {
    if( flag ) {
      flag_NG = 1;
      fprintf( stderr, "[%s] fd=%d@%s is INVALID -- NG\n", self, fd, node );
    } else {
      DBGF( "fd=%d is invalid: OK", fd );
    }
  } else {
    if( !flag ) {
      flag_NG = 1;
      fprintf( stderr, "[%s] fd=%d@%s is VALID (inode=%d) -- NG\n",
	       self, fd, node, (int) stbuf.st_ino );
    } else {
      DBGF( "fd=%d is valid: OK", fd );
    }
  }
}

struct fd_and_go {
  pthread_barrier_t barrier;
  int fd;
  volatile int go;
};

struct fd_and_go fdgo;

int main( int argc, char **argv ) {
  struct fd_and_go *fdgo_root;
  struct fd_and_go *fdgo_task;
  void *exp;
  int ntasks;
  int fd;
  int i;

  ntasks = NTASKS;
  exp    = (void*) &fdgo;
  TESTINT( pip_init( &pipid, &ntasks, &exp, PIP_MODE_PROCESS ) );

  if( ( fd = open_file() ) < 0 ) {
    fprintf( stderr, "Unable to open a file\n" );
  } else {

    fdgo.fd = fd;
    fdgo.go = 0;

    if( pipid == PIP_PIPID_ROOT ) {

      for( i=0; i<NTASKS; i++ ) {
	int err;

	pipid = i;
	err = pip_spawn( argv[0], argv, NULL, i % cpu_num_limit(),
			 &pipid, NULL, NULL, NULL );
	if( err != 0 ) {
	  fprintf( stderr, "pip_spawn(%d/%d): %s\n",
		   i, NTASKS, strerror( err ) );
	  break;
	}
	if( i != pipid ) {
	  fprintf( stderr, "pip_spawn(%d!=%d) !!!!!!\n", i, pipid );
	}
      }
      ntasks = i;
      TESTINT( pthread_barrier_init( &fdgo.barrier, NULL, ntasks+1 ) );
      fdgo.go = ntasks;
      pip_memory_barrier();
      pthread_barrier_wait( &fdgo.barrier );

      for( i=0; i<ntasks; i++ ) {
	TESTINT( pip_import( i, &exp ) );
	if( exp == NULL ) {
	  fprintf( stderr, "[ROOT] pip_import(%d) returns NULL (1)\n", i );
	} else {
	  fdgo_task = (struct fd_and_go*) exp;
	  check_fd( fdgo_task->fd, i, 0 );
	}
      }
      if( !flag_NG ) {
	fprintf( stderr, "[ROOT] checking -- ok\n" );
      }

      for( i=0; i<ntasks; i++ ) {
	DBGF( "roll call %d", i );
	TESTINT( pip_import( i, &exp ) );
	if( exp == NULL ) {
	  fprintf( stderr, "[ROOT] pip_import(%d) returns NULL (2)\n", i );
	} else {
	  fdgo_task = (struct fd_and_go*) exp;
	  fdgo_task->go = 1;
	  DBG;
	  while( 1 ) {
	    if( !fdgo_task->go ) break;
	    pause_and_yield( 10 );
	  }
	  DBG;
	}
      }
      pthread_barrier_wait( &fdgo.barrier );
      for( i=0; i<ntasks; i++ ) TESTINT( pip_wait( i, NULL ) );
      TESTINT( pip_fin() );

    } else {
      fdgo_task = &fdgo;
      TESTINT( pip_export( (void*) fdgo_task ) );

      fdgo_root = (struct fd_and_go*) exp;
      while( 1 ) {
	if( fdgo_root->go ) break;
	pause_and_yield( 10 );
      }
      ntasks = fdgo_root->go;
      pthread_barrier_wait( &fdgo_root->barrier );

      while( 1 ) {
	if( fdgo.go ) break;
	pause_and_yield( 10 );
      }
      DBG;
      check_fd( fdgo_root->fd, PIP_PIPID_ROOT, 1 );
      for( i=0; i<ntasks; i++ ) {
	DBGF( "Child %d", i );
	TESTINT( pip_import( i, &exp ) );
	if( exp == NULL ) {
	  fprintf( stderr, "pip_import(%d) returns NULL\n", i );
	} else {
	  fdgo_task = (struct fd_and_go*) exp;
	  check_fd( fdgo_task->fd, i, pipid==i );
	}
      }
      if( !flag_NG ) {
	fprintf( stderr, "[%d] checking -- ok\n", pipid );
      }
      fdgo.go = 0;
      DBG;
      pthread_barrier_wait( &fdgo_root->barrier );
    }
  }
  DBG;
  close( fd );
  return 0;
}
