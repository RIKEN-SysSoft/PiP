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

#include <test.h>

#define NITERS		(10)
#define ROOT		"ROOT"

static struct my_exp {
  pip_barrier_t		barr;
  int			fd;
} exp;

static struct my_exp *expp;

int main( int argc, char **argv ) {
  char	*fname;
  char	buff[128], check[128];
  int 	stdin, stdout;
  int 	ntasks, pipid;
  int	niters;
  int	fd, len;
  int	i, j, extval = 0;

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

  expp = &exp;;
  CHECK( pip_init(&pipid,&ntasks,(void**)&expp,0), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    CHECK( pip_barrier_init(&exp.barr,ntasks+1),   RV, return(EXIT_FAIL) );
    strcpy( buff, ROOT );
    len = strlen( buff );
    if( stdin ) {
      fd = 0;
      exp.fd = fd;
      CHECK( read( fd, check, len ),              RV<0, return(EXIT_FAIL) );
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      CHECK( strcmp( buff, check ),               RV!=0, return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	}
      }
    } else if( stdout ) {
      fd = 1;
      exp.fd = fd;
      CHECK( write( fd, buff, len ), RV<0, return(EXIT_UNTESTED) );
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	}
      }
    } else {
      CHECK( fd = open( fname, O_CREAT|O_TRUNC|O_RDWR, S_IRWXU ),
	       RV<0, return(EXIT_UNTESTED) );
      exp.fd = fd;
      CHECK( write( fd, buff, len ), RV<0, return(EXIT_UNTESTED) );
      for( i=0; i<ntasks; i++ ) {
	pipid = i;
	CHECK( pip_spawn(argv[0],argv,NULL,PIP_CPUCORE_ASIS,&pipid,
			 NULL,NULL,NULL),
	       RV,
	       return(EXIT_FAIL) );
      }
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	}
      }
      CHECK( lseek( fd, (off_t) 0, SEEK_SET ),  RV<0,  return(EXIT_FAIL) );
      CHECK( read( fd, check, len ),            RV<0,  return(EXIT_FAIL) );
      CHECK( strcmp( buff, check ),             RV!=0, return(EXIT_FAIL) );
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	  CHECK( pip_barrier_wait( &expp->barr ), RV,  return(EXIT_FAIL) );
	}
      }
      CHECK( close(fd),                         RV<0,  return(EXIT_FAIL) );
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

  } else {
    fd = expp->fd;
    sprintf( buff, "%8d", pipid );
    len = strlen( buff );
    if( stdin ) {
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	  if( j == pipid ) {
	    CHECK( read( fd, check, len ),        RV<0,  return(EXIT_FAIL) );
	    CHECK( strcmp( buff, check ),         RV!=0, return(EXIT_FAIL) );
	  }
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	}
      }
    } else if( stdout ) {
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	  if( j == pipid ) {
	    CHECK( write( fd, buff, len ),        RV<0,  return(EXIT_FAIL) );
	  }
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	}
      }
    } else {
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	  if( j == pipid ) {
	    CHECK( write( fd, buff, len ),        RV<0,  return(EXIT_FAIL) );
	  }
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	}
      }
      for( i=0; i<niters; i++ ) {
	for( j=0; j<ntasks; j++ ) {
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	  if( j == pipid ) {
	    CHECK( read( fd, check, len ),        RV<0,  return(EXIT_FAIL) );
	    CHECK( strcmp( buff, check ),         RV!=0, return(EXIT_FAIL) );
	  }
	  CHECK( pip_barrier_wait( &expp->barr ), RV,    return(EXIT_FAIL) );
	}
      }
      int flag;
      CHECK( pip_is_threaded( &flag ),            RV,    return(EXIT_FAIL) );
      if( !flag ) {
	CHECK( close(fd),                         RV<0,  return(EXIT_FAIL) );
      }
    }
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return extval;
}
