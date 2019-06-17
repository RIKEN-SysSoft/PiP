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
#include <math.h>


#define BIAS	(1000)

int *a;

int test_main( exp_t *exp ) {
  int pipid, ntasks = exp->args.ntasks;
  int *ap, flag, i, k, n;

  if( ntasks < 2 ) return 1;

  if( pip_is_debug_build() ) {
    n = 20;
  } else {
    n = 5;
  }
  a = (int*) malloc( sizeof(int) * n );

  flag = 0;
  TESTINT( pip_get_pipid( &pipid ) );
  srand( pipid );
  for( i=0; i<n; i++ ) {
    if( pipid & 0x1 ) {
      a[i] = ( BIAS * pipid ) + i;
    } else {
      a[i] = -1;;
    }
  }

  for( i=0; i<n; i++ ) {
    if( pipid & 0x1 ) {
      ap = &a[i];
      fprintf( stderr, "[%d] export[%d]\n", pipid, i );
      TESTINT( pip_named_export( (void*) ap, "a[%d]", i ) );
    } else {
      do {
	k = rand() % ntasks;
      } while( !( k & 0x1 ) );
      fprintf( stderr, "[%d] import[%d]@%d\n", pipid, i, k );
      TESTINT( pip_named_import( k, (void**) &ap, "a[%d]", i ) );
      if( *ap != ( BIAS * k ) + i ) {
	flag = 1;
	fprintf( stderr, "[%d] i=%d  %d != %d\n", pipid, i, *ap, i+BIAS );
      }
    }
    if( pipid == 0 && i % 10 == 0 ) {
      fprintf( stderr, "ITER:%d\n", i );
    }
  }
#ifdef DEBUG
  fprintf( stderr, "\n[pipid:%d] DONE\n\n", pipid );
#endif
  TESTINT( pip_barrier_wait( &exp->pbarr ) );
  return flag;
}
