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

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip_internal.h>
#include <pip_dlfcn.h>

/* locked dl* functions */

void *pip_dlopen( const char *filename, int flag ) {
  void *handle;
  pip_glibc_lock( 1 );
  handle = dlopen( filename, flag );
  pip_glibc_unlock();
  return handle;
}

void *pip_dlmopen( long lmid, const char *path, int flag ) {
  void *handle;
  pip_glibc_lock( 1 );
  handle = dlmopen( lmid, path, flag );
  pip_glibc_unlock();
  return handle;
}

int pip_dlinfo( void *handle, int request, void *info ) {
  int rv;
  pip_glibc_lock( 1 );
  rv = dlinfo( handle, request, info );
  pip_glibc_unlock();
  return rv;
}

void *pip_dlsym( void *handle, const char *symbol ) {
  void *addr;
  pip_glibc_lock( 1 );
  addr = dlsym( handle, symbol );
  pip_glibc_unlock();
  return addr;
}

int pip_dladdr( void *addr, void *info ) {
  Dl_info *dlinfo = (Dl_info*) info;
  int rv;
  pip_glibc_lock( 1 );
  rv = dladdr( addr, dlinfo );
  pip_glibc_unlock();
  return rv;
}

int pip_dlclose( void *handle ) {
  int rv = 0;
  pip_glibc_lock( 1 );
  rv = dlclose( handle );
  pip_glibc_unlock();
  return rv;
}
