/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_clone_h_
#define _pip_clone_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef struct pip_clone {
  int		flag_wrap;
  int		flag_clone;
  int		pid_clone;
  void		*stack;
} pip_clone_t;

#endif

#endif
