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

#include <test.h>

int main( int argc, char **argv ) {
  int mode, ntasks, rv;

  /* before calling pip_init(), this must fail */
  rv = pip_get_mode( &mode );
  if( rv != EPERM ) {
      fprintf( stderr, "pip_get_mode: %s", strerror( rv ) );
      return EXIT_FAIL;
  }

  ntasks = NTASKS;
  rv = pip_init( NULL, &ntasks, NULL, 0 );
  if ( rv != 0 ) {
    fprintf( stderr, "pip_init: %s", strerror( rv ) );
    return EXIT_FAIL;
  }

  /* after calling pip_init(), this must succeed */
  rv = pip_get_mode( &mode );
  if( rv != 0 ) {
      fprintf( stderr, "pip_get_mode: %s", strerror( rv ) );
      return EXIT_FAIL;
  }

  return EXIT_PASS;
}
