
extern double a[];

double foo( int idx ) {
  return a[idx];
}

double bar( double *a, int idx ) {
  return a[idx];
}
