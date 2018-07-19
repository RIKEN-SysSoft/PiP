/*
  * $RIKEN_copyright: 2018 Riken Center for Computational Sceience,
  * 	  System Software Devlopment Team. All rights researved$
  * $PIP_VERSION: Version 1.0$
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
   * \param[in] ubarr pointer to a PiP barrier structure
   * \param[in] n number of participants of this barrier
   * synchronization
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c ubarr is \c NULL or \c n is invalid
   *
   * \sa pip_universal_barrier_wait(3), pip_universal_barrier_fin(3)
   */
int pip_universal_barrier_init( pip_universal_barrier_t *ubarr, int n );
  /** @}*/

  /**
   * \brief wait on a universal barrier synchronization
   *  @{
   *
   * \param[in] ubarr pointer to a PiP barrier structure
   *
   * \note Unlike \c pip_task_barrier_wait(3), ths barrier synchronization
   * may block and can be used for both of PiP tasks and ULPs.
   * \note If this function is called by a ULP or a task having
   * ULP(s), then this call may result in context switching to the
   * other ULP.
   * \note The design of this function is prioritized for ease of use
   * and this function works not in a efficient way. So, do not use
   * this in a time critical path.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EINAVL \c ubarr is \c NULL
   *
   * \sa pip_universal_barrier_init(3), pip_universal_barrier_fin(3)
   */
int pip_universal_barrier_wait( pip_universal_barrier_t *ubarr );
  /** @}*/

  /**
   * \brief finalizea universal barrier synchronization
   *  @{
   *
   * \param[in] ubarr pointer to a PiP barrier structure
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa pip_universal_barrier_init(3), pip_universal_barrier_wait(3)
   */
int pip_universal_barrier_fin( pip_universal_barrier_t *ubarr );
  /** @}*/

/**
 * @}
 */

#endif
