#include <stdio.h>
#include<time.h>
#include<stdlib.h>
int main()
{
  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 500000;  
  FILE* f;    
  long int i,j;
  fork();
  fork();
  fork();
  for (j=1;j<999999999/12;j++)
  {
    for (i=0;i<10000000;i++)
    {
      f=fopen("testabc.txt","a");                      
      if(!f)
        printf("oops!\n");

      fprintf(f,"abc\n");
      fflush(f);
      fclose(f);
  }
    if(nanosleep(&tim , &tim2) < 0 )  
    {
      printf("failed \n");
      return -1;
    }
// nanosleep(tim); //50 millis
  }
}
