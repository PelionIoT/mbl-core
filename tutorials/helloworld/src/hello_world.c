/***************************************************/
/* User sample application printing "Hello, world" */
/***************************************************/

#include <stdio.h>
#include <unistd.h>

#define NUM_OF_PRINTS 10
#define DELAY_SEC_BETWEEN_PRINTS 2

int main (void)
{
  int i;
  for(i=0; i < NUM_OF_PRINTS; ++i)
  {
      printf("Hello, world\n");
      fflush(stdout);
      sleep(DELAY_SEC_BETWEEN_PRINTS);
  }
  
  return 0;
}
