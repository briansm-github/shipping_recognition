// pick a random 10 second sample from the test audio
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

main()
{
  FILE *fp_in, *fp_out;
  short *s;
  unsigned int size,r;

  s=(short *)malloc(400000); 
  fp_in=fopen("test.raw","r");
  fp_out=fopen("sample.raw","w");
  fseek(fp_in,0,SEEK_END); size=ftell(fp_in);
  //printf("size=%i\n",size);
  size/=2; size-=80000;
  srand(time(NULL)); // seed the RNG
  r = rand()%size; 
  printf("r=%i from %i\n",r,size);
  fseek(fp_in,r*2,SEEK_SET); // jump to middle somewhere
 
  fread( s,80000,2,fp_in);
  fwrite(s,80000,2,fp_out);
  fclose(fp_in); fclose(fp_out);
}
