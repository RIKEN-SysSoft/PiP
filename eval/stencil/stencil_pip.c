/*
 * Copyright (c) 2012 Torsten Hoefler. All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@illinois.edu>
 *
 * Modified for PiP evaluation: Atsushi Hori, 2017
 */

#define _GNU_SOURCE
#include <unistd.h>
#include "stencil_par.h"

#ifdef PIP
#define MPI_PROC_NULL		PIP_PIPID_ROOT
double *mem;
#endif

#define MPI_Wtime		gettime
int r,p;
double heat; // total heat in system

#define DBG
//#define DBG	do { printf("[%d] %d\n", r, __LINE__ ); } while(0)

#ifdef PIP
pthread_barrier_t barrier, *barrp;
#endif

int get_page_table_size( void ) {
  int ptsz = 0;
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
  return ptsz;
}

int main(int argc, char **argv) {
#ifndef PIP
  MPI_Init(&argc, &argv);
#endif
  int ptsz=0;
  int n, energy, niters;
#ifndef PIP
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
  if( ( err = pip_init( &pipid, &p, NULL, 0 ) ) != 0 ) {
    fprintf( stderr, "pip_init()=%d\n", err );
    exit( 1 );
  } else if( pipid == PIP_PIPID_ROOT ) {
    fprintf( stderr, "piprun -n <N> ./a.out\n" );
    exit( 1 );
  }
  if( pipid == 0 ) {
    barrp = &barrier;
    pthread_barrier_init( barrp, NULL, p );
  } else {
    TESTINT( pip_get_addr( 0, "barrier", (void**) &barrp ) );
  }
  n = atoi(argv[1]); // nxn grid
  energy = atoi(argv[2]); // energy to be injected per iteration
  niters = atoi(argv[3]); // number of iterations
  r = pipid;
  //printf( "[%d] heat[%d]=(%p)\n", r, r, &heat );
#endif
#ifndef PIP
  MPI_Comm shmcomm;
  MPI_Comm_split_type(comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shmcomm);
#endif

  int sp; // rank and size in shmem comm
#ifndef PIP
  MPI_Comm_size(shmcomm, &sp);
  //MPI_Comm_rank(shmcomm, &sr);
#else  /* Since PiP runs on one node */
  sp = p;
#endif

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

  if(!r) ptsz = get_page_table_size();
  DBG;

  int size = (bx+2)*(by+2); // process-local grid (including halos (thus +2))
  int szsz = 2*size*sizeof(double);

  //printf("%i %i %i\n", bx, by, szsz);

#ifndef PIP
  double *mem = NULL;
  MPI_Win win;
  MPI_Win_allocate_shared(szsz, 1, MPI_INFO_NULL, shmcomm, &mem, &win);
#else
  if( posix_memalign( (void**) &mem, 4096, szsz ) != 0 ) {
    fprintf( stderr, "Not enough memory\n" );
    exit( 1 );
  }
  memset( (void*) mem, 0, szsz );
  pthread_barrier_wait( barrp );
#endif

  double *tmp;
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
  double **memp;
  TESTINT( pip_get_addr( (north>=0?north:0), "mem", (void**) &memp ) );
  northptr = *memp;
  TESTINT( pip_get_addr( (south>=0?south:0), "mem", (void**) &memp ) );
  southptr = *memp;
  TESTINT( pip_get_addr( (east>=0?east:0), "mem", (void**) &memp ) );
  eastptr  = *memp;
  TESTINT( pip_get_addr( (west>=0?west:0), "mem", (void**) &memp ) );
  westptr  = *memp;
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

  DBG;

  // initialize three heat sources
  #define nsources 3
  int sources[nsources][2] = {{n/2,n/2}, {n/3,n/3}, {n*4/5,n*8/9}};
  int locnsources=0; // number of sources in my area
  int locsources[nsources][2]; // sources local to my rank
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

  double tb=MPI_Wtime(); // take time
#ifndef PIP
  MPI_Win_lock_all(0, win);
  for( int iter=0; iter<niters; iter++ ) {
    MPI_Win_sync(win);
    MPI_Barrier(shmcomm);
  }
#else
  for( int iter=0; iter<niters; iter++ ) {
    pthread_barrier_wait( barrp );
  }
#endif
  double t=-MPI_Wtime(); // take time
  tb = - t - tb;

  for(int iter=0; iter<niters; ++iter) {
    // refresh heat sources
    for(int i=0; i<locnsources; ++i) {
      aold[ind(locsources[i][0],locsources[i][1])] += energy; // heat source
    }

#ifndef PIP
    MPI_Win_sync(win);
    MPI_Barrier(shmcomm);
#else
    DBG;
    pthread_barrier_wait( barrp );
    DBG;
#endif
    // exchange data with neighbors
    if(north != MPI_PROC_NULL) {
      for(int i=0; i<bx; ++i) aold[ind(i+1,0)] = northptr2[ind(i+1,by)]; // pack loop - last valid region
    }
    if(south != MPI_PROC_NULL) {
      for(int i=0; i<bx; ++i) aold[ind(i+1,by+1)] = southptr2[ind(i+1,1)]; // pack loop
    }
    if(east != MPI_PROC_NULL) {
      for(int i=0; i<by; ++i) aold[ind(bx+1,i+1)] = eastptr2[ind(1,i+1)]; // pack loop
    }
    if(west != MPI_PROC_NULL) {
      for(int i=0; i<by; ++i) aold[ind(0,i+1)] = westptr2[ind(bx,i+1)]; // pack loop
    }
    DBG;

    // update grid points
    heat = 0.0;
    for(int j=1; j<by+1; ++j) {
      for(int i=1; i<bx+1; ++i) {
        anew[ind(i,j)] = aold[ind(i,j)]/2.0 +
	  ( aold[ind(i-1,j)] +
	    aold[ind(i+1,j)] +
	    aold[ind(i,j-1)] +
	    aold[ind(i,j+1)] )/4.0/2.0;
        heat += anew[ind(i,j)];
      }
    }
    //printf( "[%d] heat=%g\n", r, heat );

    // swap arrays
    tmp=anew; anew=aold; aold=tmp;
    tmp=northptr; northptr=northptr2; northptr2=tmp;
    tmp=southptr; southptr=southptr2; southptr2=tmp;
    tmp=eastptr; eastptr=eastptr2; eastptr2=tmp;
    tmp=westptr; westptr=westptr2; westptr2=tmp;

#ifdef AH
    // optional - print image
    if(iter == niters-1) printarr_par(iter, anew, n, px, py, rx, ry, bx, by, offx, offy, comm);
#endif
  }
  DBG;
#ifndef PIP
  MPI_Win_unlock_all(win);
  t+=MPI_Wtime();

  // get final heat in the system
  double rheat;
  MPI_Allreduce(&heat, &rheat, 1, MPI_DOUBLE, MPI_SUM, comm);
#else
  pthread_barrier_wait( barrp );
  t+=MPI_Wtime();

  double rheat = 0.0;
  // get final heat in the system
  double *heatp;
  int i;
  for( i=0; i<p; i++ ) {
    TESTINT( pip_get_addr( i, "heat", (void**) &heatp ) );
    //printf( "[%d] heat[%d]=%g (%p)\n", r, i, *heatp, heatp );
    rheat += *heatp;
  }
#endif
  if(!r) {
    ptsz = get_page_table_size() - ptsz;
    printf("heat: %g  time, %g, %g, %i, %i [KB]\n", rheat, t, tb, ptsz, szsz*p/1024);
  }
#ifdef AH
  if(!r) {
    printf("ASZ:%d  (%d)  PtSz: %d  SP: %d\n", szsz, szsz*p, ptsz, sp);
  }
#endif
#ifndef PIP
  MPI_Win_free(&win);
  MPI_Comm_free(&shmcomm);

  MPI_Finalize();
#else
  pip_fin();
#endif
  return 0;
}
