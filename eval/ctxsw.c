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

#define TLSMUL		(100)

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
    for( i=0; i<niters*TLSMUL; i++ ) {
      pip_load_tls( tls );
    }
    c1[j] = get_cycle_counter() - c;
    t1[j] = pip_gettime() - t;
  }
  double dn = (double) niters;
  double min0 = t0[0];
  double min1 = t1[0];
  int idx0 = 0;
  int idx1 = 0;
  for( j=0; j<NSAMPLES; j++ ) {
    printf( "[%d] ctxsw    : %g  (%lu)\n",
	    j, t0[j] / dn,          c0[j] / niters );
    printf( "[%d] load_tls : %g  (%lu)\n",
	    j, t1[j] / (dn*TLSMUL), c1[j] / (niters*TLSMUL) );
    if( min0 > t0[j] ) {
      min0 = t0[j];
      idx0 = j;
    }
    if( min1 > t1[j] ) {
      min1 = t1[j];
      idx1 = j;
    }
  }
  printf( " -- dummy tls:%p\n", (void*) tls );
  printf( "[[%d]] ctxsw    : %.3g  (%lu)\n",
	  idx0, t0[idx0] / dn,          c0[idx0] / niters );
  printf( "[[%d]] load_tls : %.3g  (%lu)\n",
	  idx1, t1[idx1] / (dn*TLSMUL), c1[idx1] / (niters*TLSMUL) );
  return 0;
}
