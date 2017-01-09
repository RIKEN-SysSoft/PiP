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

#define DEBUG
#include <pip_clone.h>
#include <pip_debug.h>

pip_clone_t pip_clone_info;

typedef
int(*clone_syscall_t)(int(*)(void*), void*, int, void*, pid_t*, void*, pid_t*);

static clone_syscall_t pip_get_clone( void ) {
  static clone_syscall_t pip_clone_orig = NULL;

  if( pip_clone_orig == NULL ) {
    pip_clone_orig = (clone_syscall_t) dlsym( RTLD_NEXT, "__clone" );
  }
  return pip_clone_orig;
}

int __clone( int(*fn)(void*), void *child_stack, int flags, void *args, ... ) {
  va_list ap;
  va_start( ap, args );
  pid_t *ptid = va_arg( ap, pid_t*);
  void  *tls  = va_arg( ap, void*);
  pid_t *ctid = va_arg( ap, pid_t*);
  clone_syscall_t clone_orig;
  int retval = -1;

  if( ( clone_orig = pip_get_clone() ) == NULL ) {
    errno = ENOSYS;
  } else if( pip_clone_info.flag_wrap ) {
    pip_clone_info.flag_wrap = 0;

#ifdef DEBUG
    int oldflags = flags;
#endif

    flags &= ~(CLONE_FS);	/* 0x00200 */
    flags &= ~(CLONE_FILES);	/* 0x00400 */
    flags &= ~(CLONE_SIGHAND);	/* 0x00800 */
    flags &= ~(CLONE_THREAD);	/* 0x10000 */
    flags &= ~0xff;
    flags |= SIGCHLD;
    flags |= CLONE_VM;
    flags |= CLONE_PTRACE;

    //    flags |= CLONE_SETTLS;

    errno = 0;
    DBGF( ">>>> clone(flags: 0x%x -> 0x%x)@%p  STACK=%p, TLS=%p",
	  oldflags, flags, fn, child_stack, tls );
    retval = clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
    DBGF( "<<<< clone()=%d (errno=%d)", retval, errno );

    if( retval > 0 ) {	/* create PID is returned */
      pip_clone_info.flag_clone = flags;
      pip_clone_info.pid_clone  = retval;
      pip_clone_info.stack      = child_stack;
    }

  } else {
    DBGF( "!!! Original clone() is used" );
    retval = clone_orig( fn, child_stack, flags, args, ptid, tls, ctid );
  }
  va_end( ap );
  return retval;
}
