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
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

  int pip_check_pie( char *path );

  void pip_info_mesg( char *format, ... );
  void pip_warn_mesg( char *format, ... );
  void pip_err_mesg( char *format, ... );

  void pip_print_maps( void );
  void pip_print_fd( int fd );
  void pip_print_fds( void );
  void pip_check_addr( char *tag, void *addr );
  void pip_print_loaded_solibs( FILE *file );
  void pip_print_dsos( void );
  double pip_gettime( void );

  void pip_task_describe( FILE *fp, const char *tag, int pipid, int flag );
  void pip_ulp_describe( FILE *fp, const char *tag, pip_ulp_t *ulp, int flag );
  void pip_ulp_queue_describe( FILE *fp, const char *tag, pip_ulp_t *queue );

#ifdef __cplusplus
}
#endif

#endif

#endif
