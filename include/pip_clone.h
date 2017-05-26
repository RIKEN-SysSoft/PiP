/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_clone_h_
#define _pip_clone_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pip_machdep.h>

#define PIP_LOCK_UNLOCKED	(0)
#define PIP_LOCK_OTHERWISE	(0xFFFFFFFF)

typedef struct pip_clone {
  pip_spinlock_t lock;	     /* lock */
  int		flag_clone;  /* clone flags set by the wrapper func */
  int		pid_clone;   /* pid os the created child task */
  void		*stack;	     /* this is just for checking stack pointer */
} pip_clone_t;

#endif

#endif
