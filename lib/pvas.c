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
 * Written by Atsushi HORI <ahori@riken.jp>, 2016
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pvas.h>

int pvas_create( pvas_common_info_t info[], int n, int *pvd ) {
  int ntasks = PIP_NTASKS_MAX;
  int err;

  if( info != NULL ) return EINVAL;
  err = pip_init( NULL, &ntasks, NULL, 0 );
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
		intptr_t *pid ) {
  int err;

  err = pip_spawn( filename, argv, envp, PIP_CPUCORE_ASIS, pvid,
		   NULL, NULL, NULL );
  if( !err ) {
    err = pip_get_id( *pvid, pid );
  }
  return err;
}

int pvas_spawn_setaffinity( int pvd,
			    int *pvid,
			    char *filename,
			    char **argv,
			    char **envp,
			    intptr_t *pid,
			    int cpu ) {
  int err;

  err = pip_spawn( filename, argv, envp, cpu, pvid, NULL, NULL, NULL );
  if( !err ) {
    err = pip_get_id( *pvid, pid );
  }
  return err;
}

int pvas_get_pvid( int *pvid ) {
  int ntasks = PIP_NTASKS_MAX;
  int err;

  err = pip_init( NULL, &ntasks, NULL, 0 );
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
