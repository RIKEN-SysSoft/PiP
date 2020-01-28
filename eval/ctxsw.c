/*
 * $RIKEN_copyright: Riken Center for Computational Sceience,
 * System Software Development Team, 2016, 2017, 2018, 2019$
 * $PIP_VERSION: Version 1.0.0$
 * $PIP_license$
 */

//#define DEBUG

#include <eval.h>
#include <test.h>

#define NSAMPLES	(10)
#define WITERS		(1000)
#define NITERS		(10*1000)

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
  double		t, t0[NSAMPLES], t1[NSAMPLES];
  uint64_t		c, c0[NSAMPLES], c1[NSAMPLES];

  ctx = pip_make_fctx( stack + STKSZ, 0, foo );
  tr  = pip_jump_fctx( ctx, NULL );

  for( j=0; j<NSAMPLES; j++ ) {
    for( i=0; i<witers; i++ ) {
      c0[j] = 0;
      t0[j] = 0.0;
      pip_gettime();
      get_cycle_counter();
      tr = pip_jump_fctx( tr.ctx, tr.data );
    }
    t = pip_gettime();
    c = get_cycle_counter();
    for( i=0; i<niters; i++ ) {
      tr = pip_jump_fctx( tr.ctx, tr.data );
    }
    c0[j] = get_cycle_counter() - c;
    t0[j] = pip_gettime() - t;

    pip_save_tls( &tls );
    for( i=0; i<witers; i++ ) {
      c1[j] = 0;
      t1[j] = 0.0;
      pip_gettime();
      get_cycle_counter();
      pip_load_tls( tls );
    }
    t = pip_gettime();
    c = get_cycle_counter();
    for( i=0; i<niters*100; i++ ) {
      pip_load_tls( tls );
    }
    c1[j] = get_cycle_counter() - c;
    t1[j] = pip_gettime() - t;
  }
  double dn = (double) niters;
  for( j=0; j<NSAMPLES; j++ ) {
    printf( "ctxsw    : %g  (%lu)\n", t0[j] / dn,     c0[j] / niters );
    printf( "load_tls : %g  (%lu)\n", t1[j] / (dn*100), c1[j] / (niters*100) );
  }
  printf( " -- dummy tls:%p\n", (void*) tls );
  return 0;
}
