/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016-2017
*/

#ifndef _pip_machdep_h_
#define _pip_machdep_h_

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#if defined(__x86_64__)
#include <pip_machdep_x86_64.h>
#elif defined(__aarch64__)
#include <pip_machdep_aarch64.h>
#else
#ifdef AHA
#error "Unsupported Machine Type"
#endif
#endif

#ifndef PIP_PAUSE
inline static void pip_pause( void ) {
  /* nop */
}
#endif

#ifndef PIP_SPIN_TRYLOCK
inline static int pip_spin_trylock (pip_spinlock_t *lock) {
  int oldval;

  oldval = __sync_lock_test_and_set(lock, 1);
  return oldval == 0;
}
#endif

#ifndef PIP_SPIN_LOCK
inline static int pip_spin_lock (pip_spinlock_t *lock) {
  while( !pip_spin_trylock( lock ) ) pip_pause();
  return 0;
}
#endif

#ifndef PIP_SPIN_UNLOCK
inline static int pip_spin_unlock (pip_spinlock_t *lock) {
  __sync_lock_release(lock);
  return 0;
}
#endif

#ifndef PIP_SPIN_INIT
inline static int pip_spin_init (pip_spinlock_t *lock) {
  pip_spin_unlock( lock );
  return 0;
}
#endif

#ifndef PIP_SPIN_DESTROY
inline static int pip_spin_destroy (pip_spinlock_t *lock) {
  /* Nothing to do.  */
  return 0;
}
#endif

#ifndef PIP_WRITE_BARRIER
inline static void pip_write_barrier(void) {
  __sync_synchronize ();
}
#endif

#ifndef PIP_MEMORY_BARRIER
inline static void pip_memory_barrier(void) {
  __sync_synchronize ();
}
#endif

#endif

#endif
