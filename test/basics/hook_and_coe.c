/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 2.0.0$
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 */
/*
 * Written by Atsushi HORI <ahori@riken.jp>
 */

#include <test.h>
#include <limits.h>
#include <libgen.h>

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(16)

#define FNAME		"GreatExpectations-Chap1.txt"

char *buf_in;
char *buf_out;

static int set_coe_flag( int fd ) {
  errno = 0;
#ifndef NO_CLOEXEC
  int flags = fcntl( fd, F_GETFD );
  if( flags >= 0 ) {
    flags |= FD_CLOEXEC;
    if( fcntl( fd, F_SETFD, flags ) < 0 ) return( errno );
    return 0;
  }
#endif
  return errno;
}

#ifdef DEBUG
static int get_coe_flag( int fd ) {
  return fcntl( fd, F_GETFD ) & FD_CLOEXEC;
}
#endif

#define PROCFD_PATH		"/proc/self/fd"
static void list_fds( void ) {
#ifdef DEBUG
  DIR *dir;
  struct dirent *direntp;
  int fd, i=0;

  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    int fd_dir = dirfd( dir );
    while( ( direntp = readdir( dir) ) != NULL ) {
      if( direntp->d_name[0] != '.' &&
	  ( fd = strtol( direntp->d_name, NULL, 10 ) ) >= 0 &&
	  fd != fd_dir ) {
	if( get_coe_flag( fd ) ) {
	  fprintf( stderr, "[%d:%d] FD:%d (%s:%ld) -- COE\n",
		   getpid(), i++, fd, direntp->d_name, direntp->d_ino );
	} else {
	  fprintf( stderr, "[%d:%d] FD:%d (%s:%ld)\n",
		   getpid(), i++, fd, direntp->d_name, direntp->d_ino );
	}
      }
    }
    (void) closedir( dir );	/* fd_dir is closed */
  }
#endif
}

static int bhook( void *hook_arg ) {
  int *pipes = (int*) hook_arg;
  //fprintf( stderr, "pipe={%d,%d}\n", pipes[0], pipes[1] );
  list_fds();
#ifdef DEBUG
  fprintf( stderr, "[%d] pipes[0]:%d pipes[1]:%d\n",
	   getpid(), pipes[0], pipes[1] );
#endif
  CHECK( close( 0 ),           RV,   return(errno) );
  CHECK( dup2(  pipes[0], 0 ), RV<0, return(errno) );
  CHECK( close( pipes[0] ),    RV,   return(errno) );
  CHECK( close( 1 ),           RV,   return(errno) );
  CHECK( dup2(  pipes[1], 1 ), RV<0, return(errno) );
  CHECK( close( pipes[1] ),    RV,   return(errno) );
  list_fds();
  return 0;
}

static int ahook( void *hook_arg ) {
  int *pipes = (int*) hook_arg;
  free( pipes );
  return 0;
}

static int pipes[PIP_NTASKS_MAX+1][2];

#define BUFSZ		(1024)

int main( int argc, char **argv ) {
  char buff[BUFSZ+1], check[BUFSZ+1], *env;
  ssize_t cc, ccr, ccw, ccc;
  size_t sz;
  char *pr, *pw, *pc;
  int pipid = 999;
  int ntasks, ntenv;
  int i;

  ntasks = 0;
  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks <= 0 ) ? NTASKS : ntasks;
  if( ( env = getenv( "NTASKS" ) ) != NULL ) {
    ntenv = strtol( env, NULL, 10 );
    if( ntasks > ntenv ) return(EXIT_UNTESTED);
  } else {
    if( ntasks > NTASKS ) return(EXIT_UNTESTED);
  }

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    int *pipep;
    int fd_in, fd_out, fd_file;

    CHECK( pipe(pipes[0]), RV, return(EXIT_FAIL ) );
    fd_out = pipes[0][1];
    CHECK( set_coe_flag( pipes[0][1] ), RV, return(EXIT_FAIL) );
    for( i=1; i<ntasks; i++ ) {
      CHECK( pipe(pipes[i]), RV, return(EXIT_FAIL ) );
    }
    CHECK( pipe(pipes[ntasks]), RV, return(EXIT_FAIL ) );
    fd_in  = pipes[ntasks][0];
    CHECK( set_coe_flag( pipes[ntasks][0] ), RV, return(EXIT_FAIL) );

#ifdef DEBUG
    {
      int ii;
      for( ii=0; ii<ntasks+1; ii++ ) {
	fprintf( stderr, "pipes[%d] = { %d, %d }\n",
		 ii, pipes[ii][0], pipes[ii][1] );
      }
    }
    fprintf( stderr, "ROOT fd_in:%d fd_out:%d\n", fd_in, fd_out );
#endif

    CHECK( pipep=(int*)malloc(sizeof(int)*2),RV==0,return(EXIT_FAIL) );
    pipep[0] = pipes[0][0];
    pipep[1] = pipes[1][1];
    pipid = 0;
    CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		      bhook, ahook, (void*) pipep ),
	   RV, return(EXIT_FAIL) );
    CHECK( close(pipes[0][0]), RV, return(EXIT_FAIL) );
    CHECK( close(pipes[1][1]), RV, return(EXIT_FAIL) );

    for( i=1; i<ntasks-1; i++ ) {
      CHECK( pipep=(int*)malloc(sizeof(int)*2),
	     RV == 0, return(EXIT_FAIL) );
      pipep[0] = pipes[i  ][0];
      pipep[1] = pipes[i+1][1];
      pipid = i;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) pipep ),
	     RV, return(EXIT_FAIL) );
      CHECK( close(pipes[i  ][0]), RV, return(EXIT_FAIL) );
      CHECK( close(pipes[i+1][1]), RV, return(EXIT_FAIL) );
    }

    if( ntasks > 1 ) {
      CHECK( pipep=(int*)malloc(sizeof(int)*2),
	     RV == 0, return(EXIT_FAIL) );
      pipep[0] = pipes[ntasks-1][0];
      pipep[1] = pipes[ntasks  ][1];
      pipid = ntasks-1;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) pipep ),
	     RV, return(EXIT_FAIL) );
      CHECK( close(pipes[ntasks-1][0]), RV, return(EXIT_FAIL) );
      CHECK( close(pipes[ntasks  ][1]), RV, return(EXIT_FAIL) );
    }

    CHECK( fd_file = open( FNAME, O_RDONLY ), RV<0, return(EXIT_FAIL) );
    while( 1 ) {
      sz = BUFSZ;
      pr = pw = buff;
      CHECK( ccr=read(fd_file,pr,sz), RV<0, return(EXIT_FAIL) );
      if( ccr == 0 ) break;
      ccc = cc = ccr;
      while( ccr > 0 ) {
	CHECK( ccw=write(fd_out,pw,ccr), RV<0, return(EXIT_FAIL) );
	ccr -= ccw;
	pw  += ccw;
      }
      memset( check, 0, sizeof(buff) );
      pc = check;
      while( ccc > 0 ) {
	CHECK( ccr=read(fd_in,pc,ccc), RV<0, return(EXIT_FAIL) );
	if( ccr == 0 ) break;
	ccc =- ccr;
	pc  += ccr;
      }
      //fprintf( stderr, "ROOT: '%s' (%d)\n", check, (int) cc );
      CHECK( strncmp( buff, check, cc ), RV, return(EXIT_FAIL) );
    }
    CHECK( close(fd_file), RV, return(EXIT_FAIL) );
    CHECK( close(fd_out),  RV, return(EXIT_FAIL) );
    CHECK( close(fd_in),   RV, return(EXIT_FAIL) );

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait( i, &status ), RV, return(EXIT_FAIL) );
    }
  } else {
    while( 1 ) {
      memset( buff, 0, sizeof(buff) );
      sz = 1024;
      pr = buff;
      CHECK( ccr=read(0,pr,sz), RV<0, return(EXIT_FAIL) );
      //fprintf( stderr, "READ: '%s' (%d)\n", pr, (int) ccr );
      if( ccr == 0 ) break;
      while( ccr > 0 ) {
	CHECK( ccw=write(1,pr,ccr), RV<0, return(EXIT_FAIL) );
	ccr -= ccw;
	pr  += ccw;
      }
    }
    CHECK( close(0), RV, return(EXIT_FAIL) );
    CHECK( close(1), RV, return(EXIT_FAIL) );
  }
  return 0;
}
