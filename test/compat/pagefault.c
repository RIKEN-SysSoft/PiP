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
#include <sys/mman.h>

struct maps {
  pip_barrier_t	barrier;
  void		*maps[NTASKS+1];
} maps;

int main( int argc, char **argv ) {
  struct maps *export = (void*) &maps;
  void 	*map;
  long 	pgsz = sysconf( _SC_PAGESIZE );
  int  	i, ntasks, ntenv, pipid;
  char	*env;

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

  CHECK( pip_init( &pipid, &ntasks, (void*) &export, 0 ),
	 RV,
	 return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    map = mmap( NULL,
		pgsz,
		PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS,
		0,
		0 );
    CHECK( (map==MAP_FAILED), RV, return(EXIT_FAIL) );
    maps.maps[ntasks] = map;

    CHECK( pip_barrier_init(&export->barrier,ntasks+1),
	   RV,
	   return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    CHECK( pip_barrier_wait(&export->barrier), RV, return(EXIT_FAIL) );
    for( i=0; i<ntasks+1; i++ ) {
      *((int*)(maps.maps[i])) = 0;
    }
    CHECK( pip_barrier_wait(&export->barrier), RV, return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      CHECK( pip_wait( i, NULL ), RV, return(EXIT_FAIL) );
    }

  } else {	/* PIP child task */
    struct maps* import = (struct maps*) export;

    map = mmap( NULL,
		pgsz,
		PROT_READ|PROT_WRITE,
		MAP_PRIVATE|MAP_ANONYMOUS,
		0,
		0 );
    CHECK( (map==MAP_FAILED), RV, return(EXIT_FAIL) );
    import->maps[pipid] = map;

    CHECK( pip_barrier_wait(&export->barrier), RV, return(EXIT_FAIL) );
    for( i=pipid; i<ntasks+1; i++ ) {
      *((int*)(import->maps[i])) = 0;
    }
    for( i=0; i<pipid; i++ ) {
      *((int*)(import->maps[i])) = 0;
    }
    CHECK( pip_barrier_wait(&export->barrier), RV, return(EXIT_FAIL) );
  }
  CHECK( munmap( map, pgsz ), RV, return(EXIT_FAIL) );
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
