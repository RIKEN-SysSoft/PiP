/*
  * $RIKEN_copyright: Riken Center for Computational Sceience,
  * System Software Development Team, 2016, 2017, 2018, 2019, 2020$
  * $PIP_VERSION: Version 2.0.0$
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

#ifndef _pip_clone_h_
#define _pip_clone_h_

#ifdef DOXYGEN_SHOULD_SKIP_THIS
#ifndef DOXYGEN_INPROGRESS
#define DOXYGEN_INPROGRESS
#endif
#endif

#ifdef DOXYGEN_INPROGRESS
#ifndef INLINE
#define INLINE
#endif
#else
#ifndef INLINE
#define INLINE			inline static
#endif
#endif

#ifndef DOXYGEN_INPROGRESS

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sched.h>
#include <pip_machdep.h>

#define PIP_LOCK_UNLOCKED	(0)
#define PIP_LOCK_OTHERWISE	(0xFFFFFFFF)

typedef
int(*clone_syscall_t)(int(*)(void*), void*, int, void*, pid_t*, void*, pid_t*);

typedef struct pip_clone {
  pip_spinlock_t lock;	     /* lock */
} pip_clone_t;

INLINE pip_spinlock_t
pip_clone_lock( pid_t tid, pip_spinlock_t *lockp ) {
  pip_spinlock_t oldval;

  while( 1 ) {
    oldval = pip_spin_trylock_wv( lockp, PIP_LOCK_OTHERWISE );
    if( oldval == tid ) {
      /* called and locked by PiP lib */
      break;
    }
    if( oldval == PIP_LOCK_UNLOCKED ) { /* lock succeeds */
      /* not called by PiP lib */
      break;
    }
  }
  return oldval;
}

INLINE void pip_clone_unlock( pip_spinlock_t *lockp ) {
  pip_spin_unlock( lockp );
}

INLINE int pip_clone_flags( int flags ) {
  flags &= ~(CLONE_FS);		/* 0x00200 */
  flags &= ~(CLONE_FILES);	/* 0x00400 */
  flags &= ~(CLONE_SIGHAND);	/* 0x00800 */
  flags &= ~(CLONE_THREAD);	/* 0x10000 */
  flags &= ~0xff;
  flags |= CLONE_VM;
  flags |= CLONE_SETTLS;  /* do not reset the CLONE_SETTLS flag */
  flags |= CLONE_PTRACE;
  flags |= SIGCHLD;
  return flags;
}

#endif	/* DOXYGEN_SHOULD_SKIP_THIS */

#endif
