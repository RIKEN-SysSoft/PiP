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

#include <pip_dlfcn.h>
#include <pip_internal.h>

/* locked dl* functions */

void *pip_dlopen( const char *filename, int flag ) {
  void *handle;
  if( pip_is_initialized() ) {
    pip_spin_lock( &pip_root->lock_ldlinux );
    handle = dlopen( filename, flag );
    pip_spin_unlock( &pip_root->lock_ldlinux );
  } else {
    handle = dlopen( filename, flag );
  }
  return handle;
}

void *pip_dlmopen( Lmid_t lmid, const char *path, int flag ) {
  void *handle;
  if( pip_is_initialized() ) {
    pip_spin_lock( &pip_root->lock_ldlinux );
    PIP_ACCUM( time_dlmopen, ( handle = dlmopen( lmid, path, flag ) ) == NULL );
    pip_spin_unlock( &pip_root->lock_ldlinux );
  } else {
    handle = pip_dlmopen( lmid, path, flag );
  }
  return handle;
}

int pip_dlinfo( void *handle, int request, void *info ) {
  int rv;
  if( pip_is_initialized() ) {
    if( !PIP_ISA_ROOT( pip_task ) ) return( -EPERM );
    pip_spin_lock( &pip_root->lock_ldlinux );
    rv = dlinfo( handle, request, info );
    pip_spin_unlock( &pip_root->lock_ldlinux );
  } else {
    rv = dlinfo( handle, request, info );
  }
  return rv;
}

void *pip_dlsym( void *handle, const char *symbol ) {
  void *addr;
  if( pip_is_initialized() ) {
    pip_spin_lock( &pip_root->lock_ldlinux );
    addr = dlsym( handle, symbol );
    pip_spin_unlock( &pip_root->lock_ldlinux );
  } else {
    addr = dlsym( handle, symbol );
  }
  return addr;
}

int pip_dladdr( void *addr, Dl_info *info ) {
  int rv;
  if( pip_is_initialized() ) {
    pip_spin_lock( &pip_root->lock_ldlinux );
    rv = dladdr( addr, info );
    pip_spin_unlock( &pip_root->lock_ldlinux );
  } else {
    rv = dladdr( addr, info );
  }
  return rv;
}

int pip_dlclose( void *handle ) {
  int rv = 0;
  if( pip_is_initialized() ) {
#ifdef PIP_DLCLOSE
    pip_spin_lock( &pip_root->lock_ldlinux );
    rv = dlclose( handle );
    pip_spin_unlock( &pip_root->lock_ldlinux );
#endif
  } else {
    rv = dlclose( handle );
  }
  return rv;
}
