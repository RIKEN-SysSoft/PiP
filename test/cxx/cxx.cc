/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
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
  * Written by Atsushi HORI <ahori@riken.jp>, 2017
*/

#include <test.h>

class PIP {
public:
  static int xxx;
  int var;
  int pipid;

  PIP(int id) { pipid = id; };
  int foo( void ) { return var; };
  void bar( int x ) { var = x; };
};

#define MAGIC		(13579)

int PIP::xxx = MAGIC;

PIP **objs;

int main( int argc, char **argv ) {
  void 	*exp;
  int pipid;
  int ntasks = 0;
  int i;

  if( argc > 1 ) {
    ntasks = strtol( argv[1], NULL, 10 );
  }
  ntasks = ( ntasks == 0 ) ? NTASKS : ntasks;

  CHECK( (objs=(PIP**)malloc(sizeof(PIP*)*ntasks)),
	 RV==0,
	 return(EXIT_UNTESTED) );
  for( i=0; i<ntasks; i++ ) {
    objs[i] = new PIP( i );
    objs[i]->bar( i * 100 );
  }

  exp = (void*) &objs[0];
  CHECK( pip_init( &pipid, &ntasks, &exp, 0 ), RV, return(EXIT_FAIL) );
  if( pipid == PIP_PIPID_ROOT ) {
    for( i=0; i<ntasks; i++ ) {
      pipid = i;
      CHECK( pip_spawn( argv[0], argv, NULL, PIP_CPUCORE_ASIS, &pipid,
			NULL, NULL, NULL ),
	     RV,
	     return(EXIT_FAIL) );
    }
    ntasks = i;
    for( i=0; i<ntasks; i++ ) {
      CHECK( pip_wait( i, NULL ), RV, return(EXIT_FAIL) );
    }
  } else {
    pip_id_t id;

    CHECK( PIP::xxx, RV!=MAGIC, return(EXIT_FAIL) );
    for( i=0; i<ntasks; i++ ) {
      CHECK( objs[i]->foo(), RV!=i*100, return(EXIT_FAIL) );
      delete objs[i];
      objs[i] = NULL;
    }
    CHECK( pip_get_system_id( PIP_PIPID_MYSELF, &id ), RV, return(EXIT_FAIL) );
  }
  CHECK( pip_fin(), RV, return(EXIT_FAIL) );
  return 0;
}
