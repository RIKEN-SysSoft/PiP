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

#ifdef PIP_EVAL
#define PIP_ACCUM(V,F)		\
  do { double __st=pip_gettime(); if(F) (V) += pip_gettime() - __st; } while(0)
#define PIP_REPORT(V)	 printf( "%s: %g\n", #V, V );
#else
#define PIP_ACCUM(V,F)		if(F)
#define PIP_REPORT(V)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  pid_t pip_gettid( void );
  int  pip_check_pie( char *path );

  void pip_info_mesg( char *format, ... );
  void pip_warn_mesg( char *format, ... );
  void pip_err_mesg(  char *format, ... );

  /* the following pip_pring_*() functions will be deprecated */
  void pip_print_maps( void );
  void pip_print_fd( int fd );
  void pip_print_fds( void );
  void pip_print_loaded_solibs( FILE *file );
  void pip_print_dsos( void );

  void pip_fprint_maps( FILE *fp );
  void pip_fprint_fd( FILE *fp, int fd );
  void pip_fprint_fds( FILE *fp );
  void pip_fprint_loaded_solibs( FILE *file );
  void pip_fprint_dsos( FILE *fp );

  void pip_check_addr( char *tag, void *addr );
  double pip_gettime( void );

  void pip_task_describe(      FILE *fp, const char *tag, int pipid );
  void pip_ulp_describe(       FILE *fp, const char *tag, pip_ulp_t *ulp );
  void pip_ulp_queue_describe( FILE *fp, const char *tag, pip_ulp_t *queue );

#ifdef __cplusplus
}
#endif

#endif

#endif
