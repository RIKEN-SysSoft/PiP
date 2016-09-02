/*
 * $RIKEN_copyright:$
 * $PIP_VERSION:$
 * $PIP_license:$
*/
/*
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PIP_INTERNAL_FUNCS
#include <pvas.h>

int pvas_create( pvas_common_info_t info[], int n, int *pvd ) {
  int ntasks = PIP_NTASKS_MAX;
  int err;

  if( info != NULL ) return EINVAL;
  err = pip_init( NULL, &ntasks, NULL, PIP_MODEL_PROCESS );
  if( !err ) {
    *pvd = 0;
  }
  return err;
}

int pvas_destroy( int pvd ) {
  return pip_fin();
}

int pvas_spawn( int pvd,
		int *pvid,
		char *filename,
		char **argv,
		char **envp,
		pid_t *pid ) {
  int err;

  err = pip_spawn( filename, argv, envp, PIP_CPUCORE_ASIS, pvid );
  if( !err ) {
    err = pip_get_pid( *pvid, pid );
  }
  return err;
}

int pvas_spawn_setaffinity( int pvd,
			    int *pvid,
			    char *filename,
			    char **argv,
			    char **envp,
			    pid_t *pid,
			    int cpu ) {
  int err;

  err = pip_spawn( filename, argv, envp, cpu, pvid );
  if( !err ) {
    err = pip_get_pid( *pvid, pid );
  }
  return err;
}

int pvas_get_pvid( int *pvid ) {
  int ntasks = PIP_NTASKS_MAX;
  int err;

  err = pip_init( NULL, &ntasks, NULL, PIP_MODEL_PROCESS );
  if( !err || err == EBUSY ) {
    return pip_get_pipid( pvid );
  }
  return err;
}

struct pip_pvas_export {
  size_t	size;
  char		exp[];
};

int pvas_ealloc(size_t esize) {
  struct pip_pvas_export *exp;

  exp = (struct pip_pvas_export*)
    malloc( sizeof( struct pip_pvas_export ) + esize );
  if( exp == NULL ) return ENOMEM;

  exp->size = esize;
  memset( exp->exp, 0, esize );
  return pip_export( (void*) exp );
}

int pvas_get_export_info(int pvid, void **addr, size_t *size) {
  struct pip_pvas_export *exp;
  int err;

  if( ( err = pip_import( pvid, (void**) &exp ) ) == 0 ) {
    if( addr != NULL ) *addr = exp->exp;
    if( size != NULL ) *size = exp->size;
  }
  return err;
}

int pvas_get_ntask( int *n ) {
  return ENOSYS;
}

int pvas_get_ncommon(int *nc) {
  return ENOSYS;
}

int pvas_get_common_info(int  n, pvas_common_info_t *info ) {
  return ENOSYS;
}
