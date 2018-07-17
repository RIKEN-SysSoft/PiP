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

#define PIP_INTERNAL_FUNCS

#define NULPS	(10)
//#define NULPS	(2)

#define DEBUG
#include <test.h>

int a;

static struct root{
  pip_ulp_t 	root;
  int 		pipid;
  int		n;
  pip_ulp_t	ulp[NULPS];
} root;

void term_cb( void *aux ) {
  struct root *rp = (struct root*) aux;
  fprintf( stderr, "PIPID:%d - terminated (%p)\n", rp->pipid, rp );
  rp->n++;
  if( rp->n < NULPS ) {
    pip_ulp_yield_to( NULL, &rp->ulp[rp->n] );
  } else {
    pip_ulp_yield_to( NULL, &rp->root );
  }
}

int main( int argc, char **argv ) {
  int i, ntasks, pipid;

  fprintf( stderr, "PID %d\n", getpid() );

  ntasks = 20;
  TESTINT( pip_init( &pipid, &ntasks, NULL, 0 ) );
  if( pipid == PIP_PIPID_ROOT ) {
    root.pipid = pipid;
    root.n     = 0;
    TESTINT( pip_make_ulp( pipid,
			   term_cb,
			   &root,
			   &root.root ) );
    for( i=0; i<NULPS; i++ ) {
      pipid = i;
      TESTINT( pip_ulp_create( argv[0],
			       argv,
			       NULL,
			       &pipid,
			       term_cb,
			       &root,
			       &root.ulp[i] ) );
    }
    TESTINT( pip_ulp_yield_to( &root.root, &root.ulp[0] ) );
  } else {
    fprintf( stderr, "<%d> Hello, ULP (stackvar@%p staticvar@%p)\n",
	     pipid, &pipid, &root );
  }
  TESTINT( pip_fin() );
  return 0;
}
