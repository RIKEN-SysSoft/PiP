/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <xpmem.h>
#include <pip_machdep.h>

#define PIP_XPMEM_SEGID_MAX		(1024)

#define PIP_XPMEM_PGSZ			(4096)

int xpmem_version(void) {
#define XPMEM_CURRENT_VERSION		(0x00026003)
  return XPMEM_CURRENT_VERSION;
}

struct pip_xpmem_st {
  pip_spinlock_t	lock;
  int			curpos;
  intptr_t		segid_tab[PIP_XPMEM_SEGID_MAX];
};

static struct pip_xpmem_st	*pip_xpmem = NULL;

int xpmem_pip_root_init( void ) {
  size_t 	sz;
  int		i;
  int		err;

  sz = sizeof(struct pip_xpmem_st);
  if( posix_memalign( (void**) &pip_xpmem, PIP_XPMEM_PGSZ, sz ) != 0 ) {
    return ENOMEM;
  }
  pip_spin_init( &pip_xpmem->lock );
  pip_xpmem->curpos = 0;
  for( i=0; i<PIP_XPMEM_SEGID_MAX; i++) pip_xpmem->segid_tab[i] = 0;

  err = pip_init( NULL, NULL,(void**)  &pip_xpmem, PIP_MODEL_PROCESS );
  if( err ) {
    free( pip_xpmem );
    pip_xpmem = NULL;
  }
  return err;
}

static int xpmem_pip_task_init( void ) {
  static int 	xpmem_pip_initialized = 0;
  int		err = 0;

  if( !xpmem_pip_initialized ) {
    xpmem_pip_initialized = 1;
    err = pip_init( NULL, NULL, (void**) &pip_xpmem, PIP_MODEL_PROCESS );
  }
  return( err );
}

xpmem_segid_t xpmem_make( void *vaddr,
			  size_t size,
			  int permit_type,
			  void *permit_value ) {
  xpmem_segid_t segid = -1;
  int curpos;
  int i;

  if( xpmem_pip_task_init() == 0 ) {

    pip_spin_lock( &pip_xpmem->lock );
    {
      curpos = pip_xpmem->curpos;
      curpos = ( curpos > PIP_XPMEM_SEGID_MAX ) ? 0 : curpos;
      if( pip_xpmem->segid_tab[curpos] == 0 ) {
	segid = curpos;
      } else {
	for( i=curpos+1; i<PIP_XPMEM_SEGID_MAX; i++ ) {
	  if( pip_xpmem->segid_tab[i] == 0 ) {
	    segid = i;
	    goto found;
	  }
	}
	for( i=0; i<=curpos; i++ ) {
	  if( pip_xpmem->segid_tab[i] == 0 ) {
	    segid = i;
	    goto found;
	  }
	}
      }
    }
  found:
    if( segid >= 0 ) {
      pip_xpmem->segid_tab[curpos] = (intptr_t) vaddr;
      pip_xpmem->curpos = segid + 1;
    }
    pip_spin_unlock( &pip_xpmem->lock );
  }
  return segid;
}

int xpmem_remove( xpmem_segid_t segid ) {
  pip_xpmem->segid_tab[segid] = 0;
  return 0;
}

xpmem_apid_t xpmem_get( xpmem_segid_t segid,
			int flags,
			int permit_type,
			void *permit_value ) {
  (void) xpmem_pip_task_init();
  return segid;
}

int xpmem_release( xpmem_apid_t apid ) {
  (void) xpmem_pip_task_init();
  return 0;
}

void *xpmem_attach( struct xpmem_addr addr, size_t size, void *vaddr ) {
  (void) xpmem_pip_task_init();
  return (void*) ( pip_xpmem->segid_tab[addr.apid] + addr.offset );
}

int xpmem_detach( void *vaddr ) {
  (void) xpmem_pip_task_init();
  return 0;
}
