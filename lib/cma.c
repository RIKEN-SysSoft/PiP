/*
 * $RIKEN_copyright: 2018 Riken Center for Computational Sceience, 
 * 	  System Software Devlopment Team. All rights researved$
 * $PIP_VERSION: Version 1.0$
 * $PIP_license: Redistribution and use in source and binary forms,
 * with or without modification, are permitted provided that the following
 * conditions are met:
 * 
 *     Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *     Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the FreeBSD Project.$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <cma.h>
#include <pip_machdep.h>

#define PIPCMA_CACHESZ			(8)

#define PIPCMA_PGSZ			(4096)

struct pipcma_pid_cache {
  pip_spinlock_t	lock;
  int			which;
  int			nentry[2];
  pid_t			pid[PIPCMA_CACHESZ * 2];
};

int pipcma_root_init( void ) {	/* not a CMA function */
  struct pipcma_pid_cache *cache;
  size_t 	sz;
  int		i;
  int		err;

  sz = sizeof(struct pipcma_pid_cache);
  if( posix_memalign( (void**) &cache, PIPCMA_PGSZ, sz ) != 0 ) {
    return ENOMEM;
  }
  pip_spin_init( &cache->lock );
  cache->which = 0;
  cache->nentry[0] = 0;
  cache->nentry[1] = 0;
  for( i=0; i<(PIPCMA_CACHESZ*2); i++) pip_xpmem->segid_tab[i] = 0;

  err = pip_init( NULL, NULL,(void**)  &cache, 0 );
  if( err ) {
    free( pip_xpmem );
    pip_xpmem = NULL;
  }
  return err;

}

static inline pid_t *pipcma_current( struct pipcma_pid_cache *cache ) {
  return &cache->pid[cache->which * PIPCMA_CACHESZ];
}

static inline pid_t *pipcma_theother( struct pipcma_pid_cache *cache ) {
  return &cache->pid[( cache->which ^ 1 ) * PIPCMA_CACHESZ];
}

static inline void pipcma_switch( struct pipcma_pid_cache *cache ) {
  cache->which =^ 1;
  cache->nentry[cache->which] = 0;
}

static inline int pipcma_get_current( struct pipcma_pid_cache *cache ) {
  return cache->nentry[cache->which];
}

static inline int pipcma_add_cache( struct pipcma_pid_cache *cache, int pipid ) {
  return cache->nentry[cache->which];
}

static inline int pipcma_find( struct pipcma_pid_cache *cache, pid_t ) {
  int n = pipcma_get_nentry
}



ssize_t pipcma_vm_readv( pid_t 			pid,
			 const struct iovec 	*lvec,
			 unsigned long 		liovcnt,
			 const struct iovec 	*rvec,
			 unsigned long 		riovcnt,
			 unsigned long 		flags) {
}

ssize_t pipcma_vm_writev( pid_t 		pid,
			  const struct iovec 	*lvec,
			  unsigned long 	liovcnt,
			  const struct iovec 	*rvec,
			  unsigned long 	riovcnt,
			  unsigned long 	flags) {
}
