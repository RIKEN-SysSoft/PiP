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

#include <ctype.h>
#include <test.h>

#define CTYPE(F,C)	(void) F((char)C)

int main( int argc, char **argv ) {
  int i;
  for( i=0; i<10000; i++ ) {
    int c;
    for( c=0; c<0x100; c++ ) {
      CTYPE( isalnum, c );
      CTYPE( isalpha, c );
      CTYPE( isascii, c );
      CTYPE( isblank, c );
      CTYPE( iscntrl, c );
      CTYPE( isdigit, c );
      CTYPE( isgraph, c );
      CTYPE( islower, c );
      CTYPE( isprint, c );
      CTYPE( ispunct, c );
      CTYPE( isspace, c );
      CTYPE( isupper, c );
      CTYPE( isxdigit, c );
    }
  }
  return 0;
}
