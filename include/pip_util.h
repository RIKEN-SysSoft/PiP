/*
  * $RIKEN_copyright:$
  * $PIP_VERSION:$
  * $PIP_license:$
*/
/*
  * Written by Atsushi HORI <ahori@riken.jp>, 2016, 2017
*/

#ifndef _pip_util_h_
#define _pip_util_h_

/************************************************************/
/* The following functions are just utilities for debugging */
/************************************************************/

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <pip.h>
#include <pip_ulp.h>

#define PIPIDLEN	(64)

#ifdef __cplusplus
extern "C" {
#endif

  void   pip_print_maps( void );
  void   pip_print_fd( int fd );
  void   pip_print_fds( void );
  void   pip_check_addr( char *tag, void *addr );
  void   pip_print_loaded_solibs( FILE *file );
  void   pip_print_dsos( void );
  void   pip_ulp_describe( pip_ulp_t *ulp );
  double pip_gettime( void );

#ifdef __cplusplus
}
#endif

#endif

#endif
