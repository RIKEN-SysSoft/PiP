/* this program was originally written by Min Si and Kaiming Ouyang (ANL)  */

#include <stdio.h>

int main(int argc, char* argv[]){
  int x=0;
  printf("x %d\n", x);
  fflush(stdout); /* Segmentation Fault Here! */
  return 0;
}
