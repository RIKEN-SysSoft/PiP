/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2018
*/

#ifndef _pip_universal_h_
#define _pip_universal_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <semaphore.h>
#include <pip.h>
#include <pip_ulp.h>
#include <pip_machdep.h>

/* universal (work on both tasks and ulps) synchronizations */

typedef struct pip_universal_barrier {
  pip_spinlock_t	lock;
  int			count_init;
  volatile int		count_barr;
  volatile int		count_sem;
  volatile int		turn;
  sem_t			semaphore[2];
  pip_ulp_queue_t	queue[2];
} pip_universal_barrier_t;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @addtogroup libpip_universal libpip_universal
 * \brief the PiP universal library
 * @{
 */

  /**
   * \brief initialize barrier universal synchronization structure
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   * \param[in] n number of participants of this barrier
   * synchronization
   *
   * \sa pip_universal_barrier_wait(3), pip_universal_barrier_fin(3)
   */
int pip_universal_barrier_init( pip_universal_barrier_t *ubarr, int n );
  /** @}*/

  /**
   * \brief wait on a universal barrier synchronization
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \note Unlike \c pip_task_barrier_wait(3), ths barrier synchronization
   * may block and can be used for both of PiP tasks and ULPs.
   * \note If this function is called by a ULP or a task having
   * ULP(s), then this call may result in context switching to the
   * other ULP.
   *
   * \sa pip_universal_barrier_init(3), pip_universal_barrier_fin(3)
   */
int pip_universal_barrier_wait( pip_universal_barrier_t *ubarr );
  /** @}*/

  /**
   * \brief finalizea universal barrier synchronization
   *  @{
   *
   * \param[in] barrp pointer to a PiP barrier structure
   *
   * \sa pip_universal_barrier_init(3), pip_universal_barrier_wait(3)
   */
int pip_universal_barrier_fin( pip_universal_barrier_t *ubarr );
  /** @}*/

/**
 * @}
 */

#endif
