/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <dlfcn.h>
#include <sched.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>

//#define DEBUG
#include <pip_clone.h>
#include <pip_debug.h>

pip_clone_t pip_clone_info;

typedef
int(*clone_syscall_t)(int(*)(void*), void*, int, void*, pid_t*, void*, pid_t*);

static int get_funcaddr( char *fname, void **faddrp ) {
    void *faddr = dlsym( RTLD_NEXT, fname );
    if( faddr == NULL ) {
      DBGF( "dlym(): %s", dlerror() );
      return ENOSYS;
    }
    *faddrp = faddr;
    return 0;
  }

int __clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  va_list ap;
  va_start( ap, args );
  pid_t *ptid = va_arg( ap, pid_t*);
  void  *tls  = va_arg( ap, void*);
  pid_t *ctid = va_arg( ap, pid_t*);
  clone_syscall_t clone_orig;
  int retval = -1;
  int err;

  if( ( err = get_funcaddr( "__clone", (void**) &clone_orig ) ) != 0 ) {
    errno = err;
  } else {
    if( pip_clone_info.flag_wrap ) {
      int __attribute__ ((unused)) oldflags = flags;

      pip_clone_info.flag_wrap = 0;

      flags &= ~CLONE_FS;	/* 0x00200?*/
      flags &= ~CLONE_FILES;	/* 0x00400 */
      flags &= ~CLONE_SIGHAND;	/* 0x00800 */
      flags &= ~CLONE_THREAD;	/* 0x10000 */
      flags &= ~0xff;
      flags |= SIGCHLD;

      DBGF( ">>>> clone(flags: 0x%x -> 0x%x)@%p", oldflags, flags, fn );
      retval = clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
      DBGF( "<<<< clone()=%d (errno=%d)", retval, errno );

      if( retval > 0 ) {	/* create PID is returned */
	pip_clone_info.flag_clone = flags;
	pip_clone_info.pid_clone  = retval;
	pip_clone_info.stack      = child_stack;
      }

    } else {
      retval = clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
    }
  }
  va_end( ap );
  return retval;
}
