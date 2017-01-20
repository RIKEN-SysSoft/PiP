
double foo( int idx ) {
  extern double a[];
  return a[idx];
}

double bar( double a[], int idx ) {
  return a[idx];
}
