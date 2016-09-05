/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#ifndef _pip_h_
#define _pip_h_

#define PIP_MODEL_ANY		(0x0)
#define PIP_MODEL_PTHREAD	(0x1)
#define PIP_MODEL_PROCESS	(0x2)

#define PIP_PIPID_ROOT		(-1)
#define PIP_PIPID_ANY		(-2)
#define PIP_PIPID_MYSELF	(-3)

#define PIP_NTASKS_MAX		(128)

#define PIP_CPUCORE_ASIS	(-1)

typedef int(*pip_spawnhook_t)(void*);

#ifdef __cplusplus
extern "C" {
#endif

  int pip_init( int *pipidp, int *ntask_maxp, void **root_expp, int opts );
  int pip_fin( void );
  int pip_spawn( char *prg, char **argv, char **envv, int coreno, int *pipidp,
		 pip_spawnhook_t before, pip_spawnhook_t after, void *hookarg);
  int pip_get_pipid( int *pipid );
  int pip_get_current_ntasks( int *ntasksp );
  int pip_export( void *exp );
  int pip_import( int pipid, void **expp );

  /* inplementation-independent functions to manage PIP tasks */
  int pip_exit(               int  retval );
  int pip_wait(    int pipid, int *retval );
  int pip_trywait( int pipid, int *retval );
  int pip_kill(    int pipid, int  signal );

#ifdef PIP_INTERNAL_FUNCS

  /* the following functions depends its implementation deeply */

#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>

  int pip_get_thread( int pipid, pthread_t *threadp );
  int pip_if_pthread( int *flagp );
  int pip_if_shared_fd( int *flagp );
  int pip_if_shared_sighand( int *flagp );

  int pip_join( int pipid );
  int pip_tryjoin( int pipid );
  int pip_timedjoin( int pipid, struct timespec *time );

  int pip_get_pid( int pipid, pid_t *pidp );

  int pip_print_loaded_solibs( FILE *file );
  char **pip_copy_vec( char *addition, char **vecsrc );

#endif

#ifdef __cplusplus
}
#endif

#endif
