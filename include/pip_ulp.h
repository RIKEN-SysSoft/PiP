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

#define PIP_ULP_INIT(L)					\
  do { PIP_ULP_NEXT(L) = (L);				\
    PIP_ULP_PREV(L) = (L); } while(0)

/**
#define PIP_ULP_NEXT(L)		(((pip_ulp_t*)(L))->next)
#define PIP_ULP_PREV(L)		(((pip_ulp_t*)(L))->prev)
#define PIP_ULP_PREV_NEXT(L)	(((pip_ulp_t*)(L))->prev->next)
#define PIP_ULP_NEXT_PREV(L)	(((pip_ulp_t*)(L))->next->prev)
**/
#define PIP_ULP_NEXT(L)		((L)->next)
#define PIP_ULP_PREV(L)		((L)->prev)
#define PIP_ULP_PREV_NEXT(L)	((L)->prev->next)
#define PIP_ULP_NEXT_PREV(L)	((L)->next->prev)

#define PIP_ULP_ENQ_FIRST(L,E)				\
  do { PIP_ULP_NEXT(E)   = PIP_ULP_NEXT(L);		\
    PIP_ULP_PREV(E)      = (L);				\
    PIP_ULP_NEXT_PREV(L) = (E);				\
    PIP_ULP_NEXT(L)      = (E); } while(0)

#define PIP_ULP_ENQ_LAST(L,E)				\
  do { PIP_ULP_NEXT(E)   = (L);				\
    PIP_ULP_PREV(E)      = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = (E);				\
    PIP_ULP_PREV(L)      = (E); } while(0)

#define PIP_ULP_DEQ(L)					\
  do { PIP_ULP_NEXT_PREV(L) = PIP_ULP_PREV(L);		\
    PIP_ULP_PREV_NEXT(L) = PIP_ULP_NEXT(L); 		\
    PIP_ULP_INIT(L); } while(0)

#define PIP_ULP_ISEMPTY(L)				\
  ( PIP_ULP_NEXT(L) == (L) && PIP_ULP_PREV(L) == (L) )

#define PIP_ULP_ENQ_LOCK(L,E,lock)			\
  do { pip_spin_lock(lock);				\
    PIP_ULP_ENQ((L),(E));				\
    pip_spin_unlock(lock); } while(0)

#define PIP_ULP_DEQ_LOCK(L,lock)			\
  do { pip_spin_lock(lock);				\
    PIP_ULP_DEQ((L));					\
    pip_spin_unlock(lock); } while(0)

#define PIP_ULP_FOREACH(L,E)					\
  for( (E)=(L)->next; (L)!=(E); (E)=PIP_ULP_NEXT(E) )

#define PIP_ULP_FOREACH_SAFE(L,E,TV)				\
  for( (E)=(L)->next, (TV)=PIP_ULP_NEXT(E);			\
       (L)!=(E);						\
       (E)=(TV), (TV)=PIP_ULP_NEXT(TV) )

#define PIP_ULP_FOREACH_SAFE_XXX(L,E,TV)			\
  for( (E)=(L), (TV)=PIP_ULP_NEXT(E); (L)!=(E); (E)=(TV) )

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * \brief print internal info of a PiP ULP
   *  @{
   *
   */
  int pip_ulp_new( pip_spawn_program_t *programp,
		   int *pipidp,
		   pip_ulp_t *sisters,
		   pip_ulp_t **newp );
  /** @}*/

  /**
   * \brief print internal info of a PiP ULP
   *  @{
   *
   */
  int pip_ulp_yield( void );
  /** @}*/

  /**
   * \brief print internal info of a PiP ULP
   *  @{
   *
   */
  int pip_ulp_retire( int extval );
  /** @}*/

  /**
   * \brief print internal info of a PiP ULP
   *  @{
   *
   */
  int pip_ulp_suspend( void );
  /** @}*/

  /**
   * \brief print internal info of a PiP ULP
   *  @{
   *
   */
  int pip_ulp_resume( pip_ulp_t *ulp, int flags );
  /** @}*/

#ifdef __cplusplus
}
#endif

#endif /* _pip_ulp_h_ */
