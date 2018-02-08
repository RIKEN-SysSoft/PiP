
#include <stdio.h>

extern int r;
extern int locsources[3][2]; // sources local to my rank

// row-major order
#define ind(i,j) (j)*(bx+2)+(i)

void heat_source(int bx, int energy, double *aold) {
  // refresh heat sources
  for(int i=0; i<3; ++i) {
    aold[ind(locsources[i][0],locsources[i][1])] += energy; // heat source
  }
}

double stencil_body(int north, int south, int west, int east,
		    int bx, int by,
		    double *anew, double *aold,
		    double *northptr, double *northptr2,
		    double *southptr, double *southptr2,
		    double *eastptr,  double *eastptr2,
		    double *westptr,  double *westptr2) {
  double heat, *tmp;

  // exchange data with neighbors
  if(north >= 0 && northptr2 != NULL) {
    for(int i=0; i<bx; ++i) aold[ind(i+1,0)] = northptr2[ind(i+1,by)];
  }
  if(south >= 0 && southptr2 != NULL) {
    for(int i=0; i<bx; ++i) aold[ind(i+1,by+1)] = southptr2[ind(i+1,1)];
  }
  if(east >= 0 && eastptr2 != NULL) {
    for(int i=0; i<by; ++i) aold[ind(bx+1,i+1)] = eastptr2[ind(1,i+1)];
  }
  if(west >= 0 && westptr2 != NULL) {
    for(int i=0; i<by; ++i) aold[ind(0,i+1)] = westptr2[ind(bx,i+1)];
  }

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
    // swap arrays
  tmp=anew; anew=aold; aold=tmp;
  tmp=northptr; northptr=northptr2; northptr2=tmp;
  tmp=southptr; southptr=southptr2; southptr2=tmp;
  tmp=eastptr; eastptr=eastptr2; eastptr2=tmp;
  tmp=westptr; westptr=westptr2; westptr2=tmp;

  return heat;
}
