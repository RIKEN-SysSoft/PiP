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

#define PIP_MIDLEN	(64)

#define ERRJ		{ DBG;                goto error; }
#define ERRJ_ERRNO	{ DBG; err=errno;     goto error; }
#define ERRJ_ERR(ENO)	{ DBG; err=(ENO);     goto error; }
#define ERRJ_CHK(FUNC)	{ if( (FUNC) ) goto error; }

#ifdef EVAL
#define ES(V,F)		\
  do { double __st=pip_gettime(); if(F) (V) += pip_gettime() - __st; } while(0)
double time_dlmopen   = 0.0;
double time_load_dso  = 0.0;
double time_load_prog = 0.0;
#define REPORT(V)	 printf( "%s: %g\n", #V, V );
#else
#define ES(V,F)		if(F)
#define REPORT(V)
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
