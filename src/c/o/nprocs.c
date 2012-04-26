#include<stdio.h>
#include<unistd.h>
int main()
{
  printf("%lu\n", sysconf(_SC_NPROCESSORS_ONLN));
  return 0;
}
