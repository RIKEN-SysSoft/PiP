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
  * Written by Atsushi HORI <ahori@riken.jp>
*/

#ifndef _pip_signal_h_
#define _pip_signal_h_

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
extern "C" {
#endif
#endif

  /** \defgroup pip-signal PiP Signaling Functions
   * @{
   * \page pip-signal PiP signaling functions
   * \description Signal manupilating functions. All functions listed
   * here are agnostic to the PiP execution mode.
   */
  /**
   * \page pip_kill pip_kill
   *
   * \brief deliver a signal to PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
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
   * \sa tkill(2)
   */
  int pip_kill( int pipid, int signal );

  /**
   * \page pip_sigmask pip_sigmask
   *
   * \brief set signal mask of the current PiP task
   *
   * \synopsis
   * \#include <pip.h> \n
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
   * \sa sigprocmask, pthread_sigmask
   */
  int pip_sigmask( int how, const sigset_t *sigmask, sigset_t *oldmask );

  /**
   * \page pip_signal_wait pip_signal_wait
   *
   * \brief wait for a signal
   *
   * \synopsis
   * \#include <pip.h> \n
   * int pip_signal_wait( int signal );
   *
   * \description
   * This function is agnostic to the PiP execution mode.
   *
   * \param[in] signal signal to wait
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \note This function does NOT return the \p EINTR error. This case
   * is treated as normal return;
   *
   * \sa sigwait, sigsuspend
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
