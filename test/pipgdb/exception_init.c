/*
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 3.0.0$
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

void foo_init( int, int );

void bar_init( int n, int sig ) {
  if( n > 0 ) foo_init( n-1, sig );
  kill( getpid(), sig );
}

void foo_init( int n, int sig ) {
  if( n > 0 ) bar_init( n-1, sig );
  kill( getpid(), sig );
}

int main( int argc, char **argv ) {
  int sig = SIGSEGV;

  if( argc > 1 ) {
    sig = strtol( argv[1], NULL, 10 );
  }
  if( sig <  SIGHUP  ||
      sig >  SIGUSR2 ||
      sig == SIGKILL ||
      sig == SIGCHLD ||
      sig == SIGSTOP ||
      sig == SIGTSTP ||
      sig == SIGCONT ) {
    return( EXIT_UNTESTED );
  }
  pip_init( NULL, NULL, NULL, 0 );
  foo_init( 5, sig );
  return 0;
}
