/*
 * $PIP_license: <Simplified BSD License>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *     Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 * $
 * $RIKEN_copyright: Riken Center for Computational Sceience (R-CCS),
 * System Software Development Team, 2016-2020
 * $
 * $PIP_VERSION: Version 3.0.0$
 *
 * $Author: Atsushi Hori (R-CCS) mailto: ahori@riken.jp or ahori@me.com
 * $
 */

#ifndef _pip_signal_h_
#define _pip_signal_h_

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
extern "C" {
#endif
#endif

  /**
   * \defgroup PiP-6-signal PiP Signaling Functions
   * @{
   * \page pip-signal PiP signaling functions
   * \description Signal manupilating functions. All functions listed
   * here are agnostic to the PiP execution mode.
   */
  /**
   * \PiPManEntry{pip_kill}
   *
   * \brief deliver a signal to PiP task
   *
   * \description
   * This function is agnostic to the PiP execution mode.
   *
   * \synopsis
   * \#include <pip/pip.h> \n
   * int pip_kill( int pipid, int signal );
   *
   * \param[out] pipid PiP ID of a target PiP task to deliver the signal
   * \param[out] signal signal number to be delivered
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   * \retval EINVAL An invalid signal number or invalid PiP ID is
   * specified
   *
   * \sa tkill(Linux 2)
   */
  int pip_kill( int pipid, int signal );

  /**
   * \PiPManEntry{pip_sigmask}
   *
   * \brief set signal mask of the current PiP task
   *
   * \synopsis
   * \#include <pip/pip.h> \n
   * int pip_sigmask( int how, const sigset_t *sigmask, sigset_t *oldmask );
   *
   * \description
   * This function is agnostic to the PiP execution mode.
   *
   * \param[in] how see \b sigprogmask or \b pthread_sigmask
   * \param[in] sigmask signal mask
   * \param[out] oldmask old signal mask
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   * \retval EINVAL An invalid signal number or invalid PiP ID is
   * specified
   *
   * \sa sigprocmask(Linux 2)
   * \sa pthread_sigmask(Linux 2)
   */
  int pip_sigmask( int how, const sigset_t *sigmask, sigset_t *oldmask );

  /**
   * \PiPManEntry{pip_signal_wait}
   *
   * \brief wait for a signal
   *
   * \synopsis
   * \#include <pip/pip.h> \n
   * int pip_signal_wait( int signal );
   *
   * \description
   * This function is agnostic to the PiP execution mode.
   *
   * \param[in] signal signal to wait
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note
   * This function does NOT return the \p EINTR error. This case
   * is treated as normal return;
   *
   * \sa sigwait(Linux 2)
   * \sa sigsuspend(Linux 2)
   */
  int pip_signal_wait( int signal );

  /**
   *   @}
   */

#ifndef DOXYGEN_INPROGRESS

  int  pip_tgkill( int, int, int );
  int  pip_tkill( int, int );

#ifdef __cplusplus
}
#endif
#endif /* DOXYGEN */

#endif	/* _pip_signal_h_ */
