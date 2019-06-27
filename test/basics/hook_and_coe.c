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
#include <libgen.h>
#include <limits.h>

#ifdef NTASKS
#undef NTASKS
#endif
#define NTASKS		(16)

#define FNAME		"GreatExpectations-Chap1.txt"

char *buf_in;
char *buf_out;

#ifndef NO_CLOEXEC
static int set_coe_flag( int fd ) {
  int flags = fcntl( fd, F_GETFD );
  if( flags >= 0 ) {
    flags |= FD_CLOEXEC;
    if( fcntl( fd, F_SETFD, flags ) < 0 ) return( errno );
    return 0;
  }
  return errno;
}
#endif

#define PROCFD_PATH		"/proc/self/fd"
static void list_fds( void ) {
#ifdef DEBUG
  int get_coe_flag( int fd ) {
    return fcntl( fd, F_GETFD ) & FD_CLOEXEC;
  }
  DIR *dir;
  struct dirent entry, *direntp;
  int fd, i=0;

  if( ( dir = opendir( PROCFD_PATH ) ) != NULL ) {
    int fd_dir = dirfd( dir );
    while( readdir_r( dir, &entry, &direntp ) == 0 && direntp != NULL ) {
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
    (void) closedir( dir );
    (void) close( fd_dir );
  }
#endif
}

static int bhook( void *hook_arg ) {
  int *pipes = (int*) hook_arg;
  //fprintf( stderr, "pipe={%d,%d}\n", pipes[0], pipes[1] );
  list_fds();
  CHECK( close( 0 ),          RV,   return(errno) );
  CHECK( dup2( pipes[0], 0 ), RV<0, return(errno) );
  CHECK( close( pipes[0] ),   RV,   return(errno) );
  CHECK( close( 1 ),          RV,   return(errno) );
  CHECK( dup2( pipes[1], 1 ), RV<0, return(errno) );
  CHECK( close( pipes[1] ),   RV,   return(errno) );
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
  char buff[BUFSZ+1];
  char check[BUFSZ+1];
  ssize_t cc, ccr, ccw, ccc;
  size_t sz;
  char *pr, *pw, *pc;
  int pipid = 999;
  int ntasks = 0;
  int i;

  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  if( ntasks < 1 || ntasks > NTASKS ) {
    ntasks = NTASKS;
  }

  CHECK( pip_init( &pipid, &ntasks, NULL, 0 ), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    char fname[PATH_MAX];
    char path[PATH_MAX];
    char *dir, *p;
    int *pipep;
    int fd_in, fd_out, fd_file;

    dir = dirname( strdup( argv[0] ) );
    p = fname;
    p = stpcpy( p, dir );
    p = stpcpy( p, "/" );
    (void) strcpy( p, FNAME );
    fprintf( stderr, "ARGV0:%s  DIR:%s  FILE:%s\n", argv[0], dir, fname );
    CHECK( realpath( fname, path ), RV==0, return(EXIT_UNTESTED) );

    CHECK( pipe(pipes[0]), RV, return(EXIT_UNTESTED ) );
    fd_out = pipes[0][1];
#ifndef NO_CLOEXEC
    CHECK( set_coe_flag( pipes[0][1] ), RV, return(EXIT_UNTESTED) );
#endif
    for( i=1; i<ntasks; i++ ) {
      CHECK( pipe(pipes[i]), RV, return(EXIT_UNTESTED ) );
#ifndef NO_CLOEXEC
      CHECK( set_coe_flag( pipes[i][0] ), RV, return(EXIT_UNTESTED) );
      CHECK( set_coe_flag( pipes[i][1] ), RV, return(EXIT_UNTESTED) );
#endif
    }
    CHECK( pipe(pipes[ntasks]), RV, return(EXIT_UNTESTED ) );
    fd_in  = pipes[ntasks][0];
#ifndef NO_CLOEXEC
    CHECK( set_coe_flag( pipes[ntasks][0] ), RV, return(EXIT_UNTESTED) );
#endif

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

    CHECK( pipep=(int*)malloc(sizeof(int)*2),
	   RV == 0,
	   return(EXIT_UNTESTED) );
    pipep[0] = pipes[0][0];
    pipep[1] = pipes[1][1];
    pipid = 0;
    CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
		      bhook, ahook, (void*) pipep ),
	   RV,
	   return(EXIT_FAIL) );
    CHECK( close(pipes[0][0]), RV, return(EXIT_UNTESTED) );
    CHECK( close(pipes[1][1]), RV, return(EXIT_UNTESTED) );

    for( i=1; i<ntasks-1; i++ ) {
      CHECK( pipep=(int*)malloc(sizeof(int)*2),
	     RV == 0,
	     return(EXIT_UNTESTED) );
      pipep[0] = pipes[i][0];
      pipep[1] = pipes[i+1][1];
      pipid = i;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) pipep ),
	     RV,
	     return(EXIT_FAIL) );
      CHECK( close(pipes[i][0]),   RV, return(EXIT_UNTESTED) );
      CHECK( close(pipes[i+1][1]), RV, return(EXIT_UNTESTED) );
    }

    if( ntasks > 1 ) {
      CHECK( pipep=(int*)malloc(sizeof(int)*2),
	     RV == 0,
	     return(EXIT_UNTESTED) );
      pipep[0] = pipes[ntasks-1][0];
      pipep[1] = pipes[ntasks][1];
      pipid = ntasks-1;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			bhook, ahook, (void*) pipep ),
	     RV,
	     return(EXIT_FAIL) );
      CHECK( close(pipes[ntasks-1][0]), RV, return(EXIT_UNTESTED) );
      CHECK( close(pipes[ntasks][1]),   RV, return(EXIT_UNTESTED) );
    }

    CHECK( fd_file = open( path, O_RDONLY ), RV<0, return(EXIT_UNTESTED) );
    while( 1 ) {
      sz = BUFSZ;
      pr = pw = buff;
      CHECK( ccr=read(fd_file,pr,sz), RV<0, return(EXIT_UNTESTED) );
      if( ccr == 0 ) break;
      ccc = cc = ccr;
      while( ccr > 0 ) {
	CHECK( ccw=write(fd_out,pw,ccr), RV<0, return(EXIT_UNTESTED) );
	ccr -= ccw;
	pw  += ccw;
      }
      memset( check, 0, sizeof(buff) );
      pc = check;
      while( ccc > 0 ) {
	CHECK( ccr=read(fd_in,pc,ccc), RV<0, return(EXIT_UNTESTED) );
	if( ccr == 0 ) break;
	ccc =- ccr;
	pc  += ccr;
      }
      //fprintf( stderr, "ROOT: '%s' (%d)\n", check, (int) cc );
      CHECK( strncmp( buff, check, cc ), RV, return(EXIT_FAIL) );
    }
    CHECK( close(fd_file), RV, return(EXIT_UNTESTED) );
    CHECK( close(fd_out),  RV, return(EXIT_UNTESTED) );
    CHECK( close(fd_in),   RV, return(EXIT_UNTESTED) );

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait( i, &status ), RV, return(EXIT_FAIL) );
    }
  } else {
    while( 1 ) {
      memset( buff, 0, sizeof(buff) );
      sz = 1024;
      pr = buff;
      CHECK( ccr=read(0,pr,sz), RV<0, return(EXIT_UNTESTED) );
      //fprintf( stderr, "READ: '%s' (%d)\n", pr, (int) ccr );
      if( ccr == 0 ) break;
      while( ccr > 0 ) {
	CHECK( ccw=write(1,pr,ccr), RV<0, return(EXIT_UNTESTED) );
	ccr -= ccw;
	pr  += ccw;
      }
    }
    CHECK( close(0), RV, return(EXIT_UNTESTED) );
    CHECK( close(1), RV, return(EXIT_UNTESTED) );
  }
  return 0;
}