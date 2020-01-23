/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

#define DEBUG

#include <test.h>

#define WITERS		(10)
#define NITERS		(1000)

#define STKSZ		(1024*1024)

char stack[STKSZ];

static void foo( pip_transfer_t tr ) {
  while( 1 ) pip_jump_fctx( tr.ctx, tr.data );
}

int main() {
  pip_ctx_p		ctx;
  pip_transfer_t 	tr;
  pip_tls_t		tls;
  int			witers = WITERS, niters = NITERS;
  int			i, j;
  double		t0, t1;

  ctx = pip_make_fctx( stack + STKSZ, 0, foo );
  tr  = pip_jump_fctx( ctx, NULL );

  for( j=0; j<10; j++ ) {
    for( i=0; i<witers; i++ ) {
      pip_gettime();
      tr = pip_jump_fctx( tr.ctx, tr.data );
    }
    t0 = - pip_gettime();
    for( i=0; i<niters; i++ ) {
      tr = pip_jump_fctx( tr.ctx, tr.data );
    }
    t0 += pip_gettime();

    for( i=0; i<witers; i++ ) {
      pip_save_tls( &tls );
      pip_load_tls( tls );
    }
    t1 = - pip_gettime();
    for( i=0; i<niters; i++ ) {
      pip_load_tls( tls );
    }
    t1 += pip_gettime();

    printf( "ctxsw    : %g\n",       t0 / ((double) niters) );
    printf( "load_tls : %g  (%p)\n", t1 / ((double) niters), (void*) tls );
  }
  return 0;
}

