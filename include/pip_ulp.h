/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_ulp_h_
#define _pip_ulp_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

typedef void (*pip_ulp_termcb_t) ( void* );

#include <ucontext.h>

typedef ucontext_t	pip_ulp_ctx_t;

typedef struct pip_ulp {
  pip_ulp_ctx_t		*ctx;
  pip_ulp_termcb_t	termcb;
  void			*aux;
  int			pipid;
} pip_ulp_t;

#define PIP_ULP_NEXT(L)		(((pip_dlist_t*)(L))->next)
#define PIP_ULP_PREV(L)		(((pip_dlist_t*)(L))->prev)
#define PIP_ULP_PREV_NEXT(L)	(((pip_dlist_t*)(L))->prev->next)
#define PIP_ULP_NEXT_PREV(L)	(((pip_dlist_t*)(L))->next->prev)

#define PIP_ULP_LIST_INIT(L)				\
  do { PIP_ULP_NEXT(L) = (pip_dlist_t*)(L);		\
    PIP_ULP_PREV(L)    = (pip_dlist_t*)(L); } while(0)

#define PIP_ULP_ENQ(L,R)						\
  do { PIP_ULP_NEXT(L)   = PIP_ULP_NEXT(R);				\
    PIP_ULP_PREV(L)      = (pip_dlist_t*)(R);				\
    PIP_ULP_NEXT(R)      = (pip_dlist_t*)(L);				\
    PIP_ULP_NEXT_PREV(R) = (pip_dlist_t*)(L); } while(0)

#define PIP_ULP_DEQ(L)						\
  do { PIP_ULP_NEXT_PREV(L) = PIP_ULP_PREV(L);			\
    PIP_ULP_PREV_NEXT(L)    = PIP_ULP_NEXT(L); } while(0)

#define PIP_ULP_NULLQ(R)	( PIP_ULP_NEXT(R) == PIP_ULP_PREV(R) )

#define PIP_ULP_ENQ_LOCK(L,R,lock)		\
  do { pip_spin_lock(lock);			\
    PIP_ULP_ENQ((L),(R));			\
    pip_spin_unlock(lock); } while(0)

#define PIP_ULP_DEQ_LOCK(L,lock)		\
  do { pip_spin_lock(lock);			\
    PIP_ULP_DEQ((L));				\
    pip_spin_unlock(lock); } while(0)

typedef struct pip_dlist {
  struct pip_dlist	*next;
  struct pip_dlist	*prev;
} pip_dlist_t;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup libpip libpip
 * \brief the PiP-ULP library
 * @{
 * @file
 * @{
 */

  int pip_ulp_create( char *prog,
		      char **argv,
		      char **envv,
		      int  *pipidp,
		      pip_ulp_termcb_t termcb,
		      void *aux,
		      pip_ulp_t *ulpp );
  int pip_make_ulp( int pipid,
		    pip_ulp_termcb_t termcb,
		    void *aux,
		    pip_ulp_t *ulp );
  int pip_ulp_yield_to( pip_ulp_t *oldulp, pip_ulp_t *newulp );
  int pip_ulp_exit( int retval );

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _pip_ulp_h_ */
