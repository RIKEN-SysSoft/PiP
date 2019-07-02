#include <stdio.h>

__thread int tlsvar;
int main() {
  printf( "%p\n", &tlsvar );
  return 0;
}
