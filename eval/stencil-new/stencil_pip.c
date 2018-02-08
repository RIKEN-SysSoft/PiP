/*
 * Copyright (c) 2012 Torsten Hoefler. All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@illinois.edu>
 *
 * Modified for PiP evaluation: Atsushi Hori, 2017
 */

#define _GNU_SOURCE
#include <sys/mman.h>
#include <numa.h>
#include <numaif.h>
#include <unistd.h>
#include <string.h>
#include "stencil_par.h"

#ifndef MPI_PROC_NULL
#define MPI_PROC_NULL		PIP_PIPID_ROOT
#endif

double *mem;

#define USE_PIP_BARRIER

#ifdef USE_PIP_BARRIER
#define BARRIER_T		pip_barrier_t
#define BARRIER_INIT(X,Y,Z)	pip_barrier_init((X),(Z))
#define BARRIER_WAIT		pip_barrier_wait
#else
#define BARRIER_T		pthread_barrier_t
#define BARRIER_INIT(X,Y,Z)	pthread_barrier_init((X),(Y),(Z))
#define BARRIER_WAIT		pthread_barrier_wait
#endif	/* USE_PIP_BARRIER */

#define MPI_Wtime		gettime
int r,p;
int npf;
double heat; // total heat in system
#define nsources 3
int locsources[nsources][2]; // sources local to my rank

#ifdef PMAP
extern void pip_print_maps( void );
#endif

#define DBG
//#define DBG	do { printf("[%d] %d\n", r, __LINE__ ); } while(0)

#ifdef PIP
BARRIER_T barrier, *barrp;
#endif

int get_page_table_size( void ) {
  int ptsz = 0;
#ifndef FAST
  FILE *fp;
  char *line = NULL;
  char keyword[128];
  size_t n = 0;

  sleep( 5 );
  if( ( fp = fopen( "/proc/meminfo", "r" ) ) != NULL ) {
    while( getline( &line, &n, fp ) > 0 ) {
      if( sscanf( line, "%s %d", keyword, &ptsz ) > 0 &&
	  strcmp( keyword, "PageTables:" ) == 0 ) break;
    }
    free( line );
    fclose( fp );
  }
  //printf( "PageTables: %d [KB]\n", ptsz );
#endif
  return ptsz;
}

int get_page_faults( void ) {
  FILE *fp;
  unsigned long npf = 0;

  if( ( fp = fopen( "/proc/self/stat", "r" ) ) != NULL ) {
    fscanf( fp,
	    "%*d "		/* pid */
	    "%*s "		/* comm */
	    "%*c "		/* state */
	    "%*d "		/* ppid */
	    "%*d "		/* pgrp */
	    "%*d "		/* session */
	    "%*d "		/* tty_ny */
	    "%*d "		/* tpgid */
	    "%*u "		/* flags */
	    "%lu ",		/* minflt */
	    &npf );
  }
  return (int) npf;
}

int main(int argc, char **argv) {
  int ptsz=0;
  int n, energy, niters;

  if( 0 ) {
    unsigned long nodemask = 0;
    int maxnode = numa_max_node();
    for( int i=0; i<=maxnode; i++ ) nodemask = ( nodemask << 1 ) | 1;
    printf( "maxnode=%d nodemask=0x%lx\n", maxnode, nodemask );
    if( set_mempolicy( MPOL_BIND, &nodemask, maxnode ) != 0 ) {
      printf( "set_mempolicy()=%d\n", errno );
    }
  }

#ifndef PIP
  MPI_Init(&argc, &argv);
  MPI_Comm comm = MPI_COMM_WORLD;
  MPI_Comm_rank(comm, &r);
  MPI_Comm_size(comm, &p);

  if (r==0) {
    // argument checking
    if(argc < 4) {
      if(!r) printf("usage: stencil_mpi <n> <energy> <niters>\n");
      MPI_Finalize();
      exit(1);
    }

    n = atoi(argv[1]); // nxn grid
    energy = atoi(argv[2]); // energy to be injected per iteration
    niters = atoi(argv[3]); // number of iterations

    // distribute arguments
    int args[3] = {n, energy, niters};
    MPI_Bcast(args, 3, MPI_INT, 0, comm);
  }
  else {
    int args[3];
    MPI_Bcast(args, 3, MPI_INT, 0, comm);
    n=args[0]; energy=args[1]; niters=args[2];
  }
#else
  int pipid, err;
  if( !pip_isa_piptask() ) p = 1;

  DBG;
  if( ( err = pip_init( &pipid, &p, NULL, 0 ) ) != 0 ) {
    fprintf( stderr, "pip_init()=%d\n", err );
    exit( 1 );
  }
  DBG;
  if( pipid == 0 ) {
    BARRIER_INIT( &barrier, NULL, p );
    barrp = &barrier;
    BARRIER_WAIT( barrp );
  } else if( p > 1 ) {
    BARRIER_T **addr;
    TESTINT( pip_get_addr( 0, "barrp", (void**) &addr ) );
    while( *addr == NULL );
    barrp = *addr;
    BARRIER_WAIT( barrp );
  }
  DBG;
  n = atoi(argv[1]); // nxn grid
  energy = atoi(argv[2]); // energy to be injected per iteration
  niters = atoi(argv[3]); // number of iterations
  if( p > 1 ) {
    r = pipid;
  } else {
    r = 0;
  }
  DBG;
  //printf( "[%d] heat[%d]=(%p)\n", r, r, &heat );
  //printf( "[%d] barrp: %p\n", r, barrp );
#endif

#ifndef PIP
  MPI_Comm shmcomm;
  MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shmcomm);
#endif
  DBG;

  int sp; // rank and size in shmem comm
#ifndef PIP
  MPI_Comm_size(shmcomm, &sp);
  //MPI_Comm_rank(shmcomm, &sr);
#else  /* Since PiP runs on one node */
  sp = p;
#endif
  DBG;

#ifdef AH
  // this code works only on comm world!
  if(sp != p)
    MPI_Abort(comm, 1);
#endif

#ifndef PIP
  int pdims[2]={0,0};
  MPI_Dims_create(sp, 2, pdims);
  int px = pdims[0];
  int py = pdims[1];
  //if(!r) printf("processor grid: %i x %i\n", px, py);
#else
  int px, py;
  switch( sp ) {
  case 1:
    px = 1;
    py = 1;
    break;
  case 2:
    px = 2;
    py = 1;
    break;
  case 4:
    px = 2;
    py = 2;
    break;
  case 8:
    px = 4;
    py = 2;
    break;
  case 16:
    px = 4;
    py = 4;
    break;
  case 32:
    px = 8;
    py = 4;
    break;
  case 64:
    px = 8;
    py = 8;
    break;
  case 128:
    px = 16;
    py = 8;
    break;
  case 256:
    px = 16;
    py = 16;
    break;
  default:
    if(!r) fprintf( stderr, "np (%d) must be poqer of 2\n", r );
    exit( 1 );
  }
#endif
  DBG;

  // determine my coordinates (x,y) -- r=x*a+y in the 2d processor array
  int rx = r % px;
  int ry = r / px;
  // determine my four neighbors
  int north = (ry-1)*px+rx; if(ry-1 < 0)   north = MPI_PROC_NULL;
  int south = (ry+1)*px+rx; if(ry+1 >= py) south = MPI_PROC_NULL;
  int west  = ry*px+rx-1;   if(rx-1 < 0)   west  = MPI_PROC_NULL;
  int east  = ry*px+rx+1;   if(rx+1 >= px) east  = MPI_PROC_NULL;
  // decompose the domain
  int bx = n/px; // block size in x
  int by = n/py; // block size in y
  int offx = rx*bx; // offset in x
  int offy = ry*by; // offset in y
  DBG;

  //printf("%i (%i,%i) - w: %i, e: %i, n: %i, s: %i\n", r, ry,rx,west,east,north,south);

  DBG;

  int size = (bx+2)*(by+2); // process-local grid (including halos (thus +2))
  long szsz = 2*size*sizeof(double);

  double ta;
#ifndef PIP
  double *mem = NULL;
  MPI_Win win;

#ifdef PMAP
  if( r == 0 ) pip_print_maps();
#endif

  MPI_Barrier(shmcomm);
  ta=-MPI_Wtime(); // take time
  MPI_Win_allocate_shared(szsz, 1, MPI_INFO_NULL, shmcomm, &mem, &win);
  memset( (void*) mem, 0, szsz );
  MPI_Barrier(shmcomm);
  ta+=MPI_Wtime();

#else
  BARRIER_WAIT( barrp );
  ta=-MPI_Wtime(); // take time
#ifdef POSIX_MEMALIGN
  if( posix_memalign( (void**) &mem, 4096, szsz ) != 0 ||
      mem == NULL ) {
    fprintf( stderr, "Not enough memory\n" );
    exit( 1 );
  }
#else
#ifdef AHAH
  {
    int fd = open( "/dev/zero", O_RDONLY );
    mem = NULL;
    if( ( mem = mmap( NULL,
		      ((szsz+4095)/4096)*4096,
		      PROT_READ|PROT_WRITE,
		      MAP_PRIVATE,
		      fd, 0 ) ) == MAP_FAILED ) {
      fprintf( stderr, "MAP_FAILD\n" );
      exit( 1 );
    }
    close( fd );
  }
#else
  {
    if( r == 0 ) {
      int fd = open( "/dev/zero", O_RDONLY );
      mem = NULL;
      if( ( mem = mmap( NULL,
			szsz*p,
			PROT_READ|PROT_WRITE,
			MAP_PRIVATE,
			fd, 0 ) ) == MAP_FAILED ) {
	fprintf( stderr, "MAP_FAILD\n" );
	exit( 1 );
      }
      close( fd );
    }
    BARRIER_WAIT( barrp );
    if( r > 0 ) {
      void **addr;
      pip_get_addr( 0, "mem", (void**) &addr );
      //printf( "[%d] addr: %p->%p\n", r, addr, *addr );
      mem = (double*) ( *addr + ( szsz * r ) );
    }
  }
#endif
#endif
  //memset( (void*) mem, 0, szsz );
  BARRIER_WAIT( barrp );
  ta+=MPI_Wtime();
#endif

#ifdef AH
  printf("[%d:%d] %i %i %i (%i) at %p--%p\n",
	 r, getpid(),
	 bx, by, szsz, szsz*p, mem, ((void*)mem)+szsz);
#endif

  double *northptr, *southptr, *eastptr, *westptr;
  double *northptr2, *southptr2, *eastptr2, *westptr2;

  double *anew=mem; // each rank's offset
  double *aold=mem+size; // second half is aold!

#ifndef PIP
  MPI_Aint sz;
  int dsp_unit;
  MPI_Win_shared_query(win, north, &sz, &dsp_unit, &northptr);
  MPI_Win_shared_query(win, south, &sz, &dsp_unit, &southptr);
  MPI_Win_shared_query(win, east, &sz, &dsp_unit, &eastptr);
  MPI_Win_shared_query(win, west, &sz, &dsp_unit, &westptr);
#else
  if( p > 1 ) {
    double **memp;
    TESTINT( pip_get_addr( (north>=0?north:0), "mem", (void**) &memp ) );
    northptr = *memp;
    TESTINT( pip_get_addr( (south>=0?south:0), "mem", (void**) &memp ) );
    southptr = *memp;
    TESTINT( pip_get_addr( (east>=0?east:0), "mem", (void**) &memp ) );
    eastptr  = *memp;
    TESTINT( pip_get_addr( (west>=0?west:0), "mem", (void**) &memp ) );
    westptr  = *memp;
  } else {
    northptr = NULL;
    southptr = NULL;
    eastptr  = NULL;
    westptr  = NULL;
  }
#endif

#ifdef AH
  printf( "[%d] mem  %p--%p\n", r, mem, mem+size );
  printf( "[%d:%d] northptr: %p--%p\n", r, north, northptr, northptr+size );
  printf( "[%d:%d] southptr: %p--%p\n", r, south, southptr, southptr+size );
  printf( "[%d:%d] eastptr:  %p--%p\n", r, east,  eastptr,  eastptr+size  );
  printf( "[%d:%d] westptr:  %p--%p\n", r, west,  westptr,  westptr+size  );
#endif

  northptr2 = northptr+size;
  southptr2 = southptr+size;
  eastptr2  = eastptr+size;
  westptr2  = westptr+size;

  if( north == MPI_PROC_NULL ) north = -1;
  if( south == MPI_PROC_NULL ) south = -1;
  if( east  == MPI_PROC_NULL ) east  = -1;
  if( west  == MPI_PROC_NULL ) west  = -1;

  DBG;

  // initialize three heat sources
  int sources[nsources][2] = {{n/2,n/2}, {n/3,n/3}, {n*4/5,n*8/9}};
  int locnsources=0; // number of sources in my area
  DBG;
  for (int i=0; i<nsources; ++i) { // determine which sources are in my patch
    int locx = sources[i][0] - offx;
    int locy = sources[i][1] - offy;
    if(locx >= 0 && locx < bx && locy >= 0 && locy < by) {
      locsources[locnsources][0] = locx+1; // offset by halo zone
      locsources[locnsources][1] = locy+1; // offset by halo zone
      locnsources++;
    }
  }

#ifndef PIP
  MPI_Barrier(shmcomm);
#else
  BARRIER_WAIT( barrp );
#endif
  double tb = MPI_Wtime(); // take time
#ifndef PIP
  MPI_Win_lock_all(0, win);
  for( int iter=0; iter<niters; iter++ ) {
    MPI_Win_sync(win);
    MPI_Barrier(shmcomm);
  }
#else
  for( int iter=0; iter<niters; iter++ ) {
    BARRIER_WAIT( barrp );
  }
  DBG;
#endif
  double t = MPI_Wtime(); // take time
  tb = t - tb;

  if(!r) ptsz = get_page_table_size();
  npf = get_page_faults();

#ifdef PMAP
  if( r == 0 ) pip_print_maps();
#endif
  t = - MPI_Wtime(); // take time
  for(int iter=0; iter<niters; ++iter) {
    void heat_source(int, int, double*);
    double stencil_body(int, int, int, int, int, int, double*, double*,
			double*, double*, double*, double*,
			double*, double*, double*, double*);
    // refresh heat sources
    heat_source(bx, energy, aold);

#ifndef PIP
    MPI_Win_sync(win);
    MPI_Barrier(shmcomm);
#else
    DBG;
    BARRIER_WAIT( barrp );
    DBG;
#endif
    heat = stencil_body(north, south, west, east,
			bx, by,
			anew, aold,
			northptr, northptr2, southptr, southptr2,
			eastptr, eastptr2, westptr, westptr2);

    //printf( "[%d] heat=%g\n", r, heat );
  }
  DBG;
#ifndef PIP
  MPI_Win_unlock_all(win);
  t+=MPI_Wtime();
  //printf( "[%d] npf %d\n", r, npf );
  npf = get_page_faults() - npf;
  if(!r) ptsz = get_page_table_size() - ptsz;
#ifdef PMAP
  if( r == 0 ) pip_print_maps();
#endif

  // get final heat in the system
  double rheat;
  MPI_Allreduce(&heat, &rheat, 1, MPI_DOUBLE, MPI_SUM, comm);

  int tnpf = 0;
  MPI_Allreduce(&npf, &tnpf, 1, MPI_INT, MPI_SUM, comm);
#else
  BARRIER_WAIT( barrp );
  t+=MPI_Wtime();
  //printf( "[%d] npf %d\n", r, npf );
  npf = get_page_faults() - npf;
  BARRIER_WAIT( barrp );

  double rheat = 0.0;
  // get final heat in the system
  double *heatp;
  int i;
  if( p > 1 ) {
    for( i=0; i<p; i++ ) {
      TESTINT( pip_get_addr( i, "heat", (void**) &heatp ) );
      //printf( "[%d] heat[%d]=%g (%p)\n", r, i, *heatp, heatp );
      rheat += *heatp;
    }
  } else {
    rheat = heat;
  }
  int tnpf=0, *npfp;
  if( p > 1 ) {
    for( i=0; i<p; i++ ) {
      TESTINT( pip_get_addr( i, "npf", (void**) &npfp ) );
      tnpf += *npfp;
    }
  } else {
    tnpf = npf;
  }
#endif
  //printf( "npf %d\n", npf );
  if(!r) {
    printf("heat: %g  time, %g, %g, %g, %i, %i, [KB], %i\n", rheat, t, ta, tb, ptsz, szsz*p/1024, tnpf);
  }
#ifdef AH
  if(!r) {
    printf("ASZ:%d  (%d)  PtSz: %d  SP: %d\n", szsz, szsz*p, ptsz, sp);
  }
#endif
  //printf( "PF %d\n", get_page_faults() );
#ifndef PIP
  MPI_Win_free(&win);
  MPI_Comm_free(&shmcomm);

  MPI_Finalize();
#else
  BARRIER_WAIT( barrp );
  pip_fin();
#endif
  return 0;
}
