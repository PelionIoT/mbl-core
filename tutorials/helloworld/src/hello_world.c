#include <stdio.h>
#include <unistd.h>

int main (void)
{
  int i;
  for(i=0; i < 10; ++i)
  {
      printf("Hello, world\n");
      sleep(2);
  }
  
  return 0;
}
