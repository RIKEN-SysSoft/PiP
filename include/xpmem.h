/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _XPMEM_H
#define _XPMEM_H

#include <linux/types.h>
#include <sys/types.h>

#include <pip.h>

typedef __s64 xpmem_segid_t;	/* segid returned from xpmem_make() */
typedef __s64 xpmem_apid_t;	/* apid returned from xpmem_get() */

struct xpmem_addr {
  xpmem_apid_t	apid;		/* apid that represents memory */
  off_t 	offset;		/* offset into apid's memory */
};

#ifdef __cplusplus
extern "C" {
#endif

  int xpmem_pip_init( void );	/* not an XPMEM function */

  int xpmem_version(void);
  xpmem_segid_t xpmem_make(void *vaddr, size_t size, int permit_type,
			   void *permit_value);
  int xpmem_remove(xpmem_segid_t segid);
  xpmem_apid_t xpmem_get(xpmem_segid_t segid, int flags, int permit_type,
		       void *permit_value);
  int xpmem_release(xpmem_apid_t apid);
  void *xpmem_attach(struct xpmem_addr addr, size_t size, void *vaddr);
  int xpmem_detach(void *vaddr);

#ifdef __cplusplus
}
#endif

#endif
