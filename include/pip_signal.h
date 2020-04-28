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

#ifdef DOXYGEN_SHOULD_SKIP_THIS
#ifndef DOXYGEN_INPROGRESS
#define DOXYGEN_INPROGRESS
#endif
#endif

#ifndef DOXYGEN_INPROGRESS
#ifdef __cplusplus
extern "C" {
#endif
#endif

/**
 * @addtogroup libpip libpip
 * \brief the PiP library
 * @{
 */

  /**
   * \brief deliver a signal to a PiP task
   *  @{
   * \param[out] pipid PiP ID of a target PiP task
   * \param[out] signal signal number to be delivered
   *
   * \note Only the PiP task can be the target of the signal delivery.
   * \note This function can be used regardless to the PiP execution
   * mode.
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   * \retval EINVAL An invalid signal number or invalid PiP ID is
   * specified
   */
  int pip_kill( int pipid, int signal );
  /** @}*/

  /**
   * \brief set signal mask of the current PiP task
   *  @{
   * \param[in] how see \b sigprogmask or \b pthread_sigmask
   * \param[in] sigmask signal mask
   * \param[out] oldmask old signal mask
   *
   * \return Return 0 on success. Return an error code on error.
   * \retval EPERM PiP library is not yet initialized
   * \retval EINVAL An invalid signal number or invalid PiP ID is
   * specified
   *
   * \sa \b sigprocmask, \b pthread_sigmask
   */
  int pip_sigmask( int how, const sigset_t *sigmask, sigset_t *oldmask );
  /** @}*/

  /**
   * \brief wait for a signal
   *  @{
   * \param[in] signal signal to wait
   *
   * \return Return 0 on success. Return an error code on error.
   *
   * \sa \b sigwait, \b sigsuspend
   */
  int pip_signal_wait( int signal );

#ifndef DOXYGEN_INPROGRESS

  int  pip_tgkill( int, int, int );
  int  pip_tkill( int, int );

#ifdef __cplusplus
}
#endif
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @}
 */

#endif	/* _pip_signal_h_ */
