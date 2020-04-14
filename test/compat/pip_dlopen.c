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

#include <libgen.h>
#include <limits.h>
#include <test.h>

#define LIBNAME 	"libnull.so"

void *pip_dlopen( const char*, int );
void *pip_dlsym( void*, const char* );
int   pip_dlclose( void* );

int main( int argc, char **argv ) {
  char path[PATH_MAX], *dir, *p;
  void *handle;
  int(*foo)(void);

  CHECK( pip_init(NULL,NULL,NULL,0), RV, return(EXIT_FAIL) );

  dir = dirname( strdup( argv[0] ) );
  p = path;
  p = stpcpy( p, dir );
  p = stpcpy( p, "/../" );
  (void) strcpy( p, LIBNAME );

  fprintf( stderr, "path:%s\n", path );
  CHECK( handle = pip_dlopen( path, RTLD_LAZY ),
	 handle==NULL,
	 return(EXIT_FAIL) );
  CHECK( ( foo = pip_dlsym( handle, "foo" ) ), foo==0, return(EXIT_FAIL) );
  CHECK( foo(),                                    RV, return(EXIT_FAIL) );
  //CHECK( pip_dlclose( handle ),                    RV, return(EXIT_FAIL) );
  return 0;
}
