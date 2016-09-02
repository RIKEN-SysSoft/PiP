/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _PIP_CMA_H
#define _PIP_CMA_H

#include <sys/types.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C" {
#endif

  int pipcma_root_init( void );	/* not a CMA function */

  ssize_t pipcma_vm_readv( pid_t 		pid,
			   const struct iovec 	*lvec,
			   unsigned long	liovcnt,
			   const struct iovec 	*rvec,
			   unsigned long	riovcnt,
			   unsigned long	flags);

  ssize_t pipcma_vm_writev( pid_t 		pid,
			    const struct iovec 	*lvec,
			    unsigned long 	liovcnt,
			    const struct iovec 	*rvec,
			    unsigned long 	riovcnt,
			    unsigned long 	flags);

#ifdef __cplusplus
}
#endif

#endif
