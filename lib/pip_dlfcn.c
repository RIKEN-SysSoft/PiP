/*
 * $PIP_license:$
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

/* the EVAL define symbol is to measure the time for calling dlmopen() */
//#define EVAL

#include <pip_internal.h>
#include <pip_dlfcn.h>

/* locked dl* functions */

void *pip_dlopen( const char *filename, int flag ) {
  void *handle;
  pip_glibc_lock();
  handle = dlopen( filename, flag );
  pip_glibc_unlock();
  return handle;
}

void *pip_dlmopen( long lmid, const char *path, int flag ) {
  void *handle;
  pip_glibc_lock();
  handle = dlmopen( lmid, path, flag );
  pip_glibc_unlock();
  return handle;
}

int pip_dlinfo( void *handle, int request, void *info ) {
  int rv;
  pip_glibc_lock();
  rv = dlinfo( handle, request, info );
  pip_glibc_unlock();
  return rv;
}

void *pip_dlsym( void *handle, const char *symbol ) {
  void *addr;
  pip_glibc_lock();
  addr = dlsym( handle, symbol );
  pip_glibc_unlock();
  return addr;
}

int pip_dladdr( void *addr, void *info ) {
  Dl_info *dlinfo = (Dl_info*) info;
  int rv;
  pip_glibc_lock();
  rv = dladdr( addr, dlinfo );
  pip_glibc_unlock();
  return rv;
}

int pip_dlclose( void *handle ) {
  int rv = 0;
  pip_glibc_lock();
  rv = dlclose( handle );
  pip_glibc_unlock();
  return rv;
}

char *pip_dlerror( void ) {
  char *dlerr;
  pip_glibc_lock();
  dlerr = dlerror();
  pip_glibc_unlock();
  return dlerr;
}
