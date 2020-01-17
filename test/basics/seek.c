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
 * Written by Atsushi HORI <ahori@riken.jp>
 */

//#define DEBUG
#include <test.h>

#define NITERS		(10)
#define ROOT		"ROOT"

#define USE_BARRIER

static struct my_exp {
#ifndef USE_BARRIER
  pip_atomic_t		sync;
#else
  pthread_barrier_t	barr;
#endif
  int			fd;
} exp;

static struct my_exp *expp;

static int my_read( int fd, char *buff, size_t len ) {
  char *p = buff;
  int cc, l = len;

  while( l > 0 ) {
    cc = read( fd, p, l );
    if( cc <= 0 ) {
      if( l == len ) return cc;
      break;
    }
    p += cc;
    l -= cc;
  }
  return (int)( p - buff );
}

static int my_write( int fd, char *buff, size_t len ) {
  char *p = buff;
  int cc, l = len;

  while( l > 0 ) {
    cc = write( fd, p, l );
    if( cc <= 0 ) {
      if( l == len ) return cc;
      break;
    }
    p += cc;
    l -= cc;
  }
  return (int)( p - buff );
}

int main( int argc, char **argv ) {
  char	*fname;
  char	buff[128], check[128];
  int 	stdin, stdout;
  int 	ntasks, pipid;
  int	niters;
  int	fd, len;
  int	i, ii, extval = 0;

  set_sigsegv_watcher();

  stdin  = 0;
  stdout = 0;
  fname  = NULL;
  if( argc > 1 ) {
    if( strcmp( argv[1], "stdin"  ) == 0 ) {
      stdin  = 1;
      stdout = 0;
    } else if( strcmp( argv[1], "stdout" ) == 0 ) {
      stdin  = 0;
      stdout = 1;
    } else {
      fname = argv[1];
    }
  } else {
    exit( EXIT_UNTESTED );
  }

  ntasks = 0;
  if( argc > 2 ) {
    ntasks = strtol( argv[2], NULL, 10 );
  }
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  niters = 0;
  if( argc > 3 ) {
    niters = strtol( argv[3], NULL, 10 );
  }
  niters = ( niters == 0 ) ? NITERS : niters;

  memset( buff,  0, sizeof(buff)  );
  memset( check, 0, sizeof(check) );


  expp = &exp;
  CHECK( pip_init(&pipid,&ntasks,(void**)&expp,0), RV,   return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    /* the FD opened by the root process are inherited to child tasks */
#ifndef USE_BARRIER
    expp->sync = -1;
#else
    CHECK( pthread_barrier_init( &expp->barr, NULL, ntasks+1 ),
      RV,
      return(EXIT_FAIL) );
#endif
    sprintf( buff, "%8s", ROOT );
    len = strlen( buff );
    if( stdin ) {
      CHECK( fd = open( "/dev/stdin", O_RDONLY ), RV<0,  return(EXIT_UNTESTED) );
      expp->fd = fd;
      CHECK( my_read( fd, check, len ),           RV<0,  return(EXIT_FAIL) );
      CHECK( strcmp( buff, check ),               RV!=0, return(EXIT_FAIL) );
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	expp->sync = 0;
	while( expp->sync < ntasks );
#else
	for( ii=0; ii<ntasks+1; ii++ ) pthread_barrier_wait( &expp->barr );
#endif
      }
    } else if( stdout ) {
      CHECK( fd = open( "/dev/stdout", O_WRONLY ), RV<0, return(EXIT_UNTESTED) );
      expp->fd = fd;
      CHECK( my_write( fd, buff, len ), RV<0, return(EXIT_UNTESTED) );
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	expp->sync = 0;
	while( expp->sync < ntasks );
#else
	for( ii=0; ii<ntasks+1; ii++ ) pthread_barrier_wait( &expp->barr );
#endif
      }
    } else {			/* file */
      CHECK( fd = open( fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU ),
	       RV<0, return(EXIT_UNTESTED) );
      expp->fd = fd;
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      for( i=0; i<niters; i++ ) {
	//fprintf( stderr, "[ROOT]:%d write '%s'\n", i, buff );
	CHECK( my_write( fd, buff, len ),      RV<0,  return(EXIT_UNTESTED) );
	CHECK( fsync(fd),                      RV!=0, return(EXIT_UNRESOLVED) );
#ifndef USE_BARRIER
	expp->sync = 0;
	while( expp->sync < ntasks );
#else
	for( ii=0; ii<ntasks+1; ii++ ) pthread_barrier_wait( &expp->barr );
#endif
      }
      //system( "echo '>>>'; cat seek-file.text; echo; echo '<<<';" );
      CHECK( lseek( fd, (off_t) 0, SEEK_SET ), RV<0,  return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	CHECK( my_read( fd, check, len ),      RV<0,  return(EXIT_FAIL) );
	fprintf( stderr, "[ROOT]:%d read '%s':'%s'\n", i, buff, check );
	CHECK( strcmp( buff, check ),          RV!=0, return(EXIT_FAIL) );
#ifndef USE_BARRIER
	expp->sync = 0;
	while( expp->sync < ntasks );
#else
	for( ii=0; ii<ntasks+1; ii++ ) pthread_barrier_wait( &expp->barr );
#endif
      }
    }

    for( i=0; i<ntasks; i++ ) {
      int status;
      CHECK( pip_wait_any( NULL, &status ), RV, return(EXIT_FAIL) );
      if( WIFEXITED( status ) ) {
	CHECK( ( extval = WEXITSTATUS( status ) ),
	       RV,
	       return(EXIT_FAIL) );
      } else {
	CHECK( "Task is signaled", RV, return(EXIT_UNRESOLVED) );
      }
    }

  } else {			/* child tasks */
    fd = expp->fd;
    sprintf( buff, "%8d", pipid );
    len = strlen( buff );

    if( stdin ) {
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	while( expp->sync != pipid ) pip_system_yield();
#else
	for( ii=0; ii<pipid+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
	CHECK( my_read( fd, check, len ),     RV<=0, return(EXIT_FAIL) );
	CHECK( strcmp( buff, check ),         RV!=0, return(EXIT_FAIL) );
#ifndef USE_BARRIER
	pip_atomic_fetch_and_add( &expp->sync, 1 );
#else
	for( ii=pipid+1; ii<ntasks+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
      }
    } else if( stdout ) {
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	while( expp->sync != pipid ) pip_system_yield();
#else
	for( ii=0; ii<pipid+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
	CHECK( my_write( fd, buff, len ),     RV<=0, return(EXIT_FAIL) );
#ifndef USE_BARRIER
	pip_atomic_fetch_and_add( &expp->sync, 1 );
#else
	for( ii=pipid+1; ii<ntasks+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
      }
    } else {			/* file */
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	while( expp->sync != pipid ) pip_system_yield();
#else
	for( ii=0; ii<pipid+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
	//fprintf( stderr, "[%d]:%d write '%s'\n", pipid, i, buff );
	CHECK( my_write( fd, buff, len ),     RV<=0, return(EXIT_FAIL) );
	CHECK( fsync(fd),                     RV!=0, return(EXIT_UNRESOLVED) );
#ifndef USE_BARRIER
	pip_atomic_fetch_and_add( &expp->sync, 1 );
#else
	for( ii=pipid+1; ii<ntasks+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
      }
      for( i=0; i<niters; i++ ) {
#ifndef USE_BARRIER
	while( expp->sync != pipid ) pip_system_yield();
#else
	for( ii=0; ii<pipid+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
	CHECK( my_read( fd, check, len ),     RV<=0, return(EXIT_FAIL) );
	fprintf( stderr, "[%d]:%d read '%s':'%s'\n", pipid, i, buff, check );
	CHECK( strcmp( buff, check ),         RV!=0, return(EXIT_FAIL) );
#ifndef USE_BARRIER
	pip_atomic_fetch_and_add( &expp->sync, 1 );
#else
	for( ii=pipid+1; ii<ntasks+1; ii++ ) {
	  pthread_barrier_wait( &expp->barr );
	}
#endif
      }
    }
  }
  int flag;
  CHECK( pip_is_threaded( &flag ),            RV,    return(EXIT_FAIL) );
  if( !flag ) {
    CHECK( close(fd),                         RV<0,  return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV,   return(EXIT_FAIL) );
  return extval;
}
