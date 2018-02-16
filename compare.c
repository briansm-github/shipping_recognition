// simplest compare - correlate all frames.
#include <stdio.h>
#include <stdlib.h>

#define TOL 105000 // normally 250000
#define FSIZE 9

main()
{
  FILE *fp, *fp_raw;
  int *cb, *f,fsize,cbsize,posn,fend,cend,fstart,cstart;
  int i,n,c,ok,chunk=0;
  int d,dist,dist1,dist2;
  int bestfstart, bestfend, bestfsize;
  int bestcstart,bestcend,best;
  int laststart=0;
  char fname[100];
  short s[100000];
  
  // read all frames of training data....
  fp=fopen("training.fe","r");
  fseek(fp,0,SEEK_END); cbsize=ftell(fp); rewind(fp); 
  cb=malloc(cbsize); fread(cb,cbsize,1,fp); fclose(fp); cbsize/=4;

  // read all frames of test sample....
  fp=fopen("sample.fe","r");
  fseek(fp,0,SEEK_END); fsize=ftell(fp); rewind(fp); 
  f=malloc(fsize); fread(f,fsize,1,fp); fclose(fp); fsize/=4;

  // now go through input looking for largest correlations within TOLERANCE
  posn=0;
  while(posn<fsize) {
    printf("posn=%i, size=%i\n",posn,fsize);
    bestfsize=-1;
    for (c=0; c<cbsize; c+=FSIZE) {
      //printf("c=%i\n",c);
      cend=0; fend=0; ok=1;
      while(ok) {
        dist=0; dist1=0; dist2=0;
	for (i=0; i<FSIZE; i++) {
	  d=f[posn+fend+i]-cb[c+cend+i]; dist+=d*d;
	  d=f[posn+fend+FSIZE+i]-cb[c+cend+i]; dist1+=d*d;
	  d=f[posn+fend+i]-cb[c+cend+FSIZE+i]; dist2+=d*d;	  
	}
	//dist1=999999999; dist2=999999999; 
	if (dist>TOL && dist1>TOL && dist2>TOL) ok=0; else
	if (dist<=dist1 && dist<=dist2) {fend+=FSIZE; cend+=FSIZE;} else
	if (dist1<dist && dist1<dist2)  {fend+=FSIZE*2; cend+=FSIZE;} else
				        {fend+=FSIZE; cend+=FSIZE*2;}
	
	if (posn+fend>fsize) ok=0; // prevent over-run!
      }
      // now try and increase the size of this match backwards.....
      fstart=0; cstart=0; ok=1; 
      if (c==0 || posn==0) ok=0;
       ok=0; // skip back search?
      while(ok) {
        dist=0; dist1=0; dist2=0;
	for (i=0; i<FSIZE; i++) {
	  d=f[posn-fstart+i]-cb[c-cstart+i]; dist+=d*d;
	  d=f[posn-fstart-FSIZE+i]-cb[c-cstart+i]; dist1+=d*d;
	  d=f[posn-fstart+i]-cb[c-cstart-FSIZE+i]; dist2+=d*d;	  
	}
	if (dist>TOL && dist1>TOL && dist2>TOL) ok=0; else
	if (dist<=dist1 && dist<=dist2) {fstart+=FSIZE; cstart+=FSIZE;} else
	if (dist1<dist && dist1<dist2)  {fstart+=FSIZE*2; cstart+=FSIZE;} else
				        {fstart+=FSIZE; cstart+=FSIZE*2;}
	
	if (posn-fstart<FSIZE) ok=0; // prevent over-run!
	if (c-cstart<FSIZE) ok=0;
	if (fstart>10*FSIZE) ok=0; // maximum back search
      }
      if (fend+fstart>bestfsize)
           {bestfstart=fstart; bestfend=fend; bestfsize=fend+fstart;
	    bestcend=cend+cstart; best=c-cstart;}
    }
    laststart=posn-bestfstart;
    chunk++;
    printf("chunk %i: best=%i, size=%i\n",chunk,best,bestfsize/FSIZE);
    
    sprintf(fname,"match_%i.raw",chunk); 
    //sprintf(fname,"match_%.3i.raw",chunk); 
    fp_raw=fopen("training.raw","r");
    fp=fopen(fname,"w");
    fseek(fp_raw,best/FSIZE*80*2,SEEK_SET);
    fread(s,bestcend/FSIZE*80,2,fp_raw);
    fwrite(s,bestcend/FSIZE*80,2,fp); fclose(fp); fclose(fp_raw);
    // write out original chunk to compare.....
    
    sprintf(fname,"sample_%i.raw",chunk);
    //sprintf(fname,"sample_%.3i",chunk); 
    fp_raw=fopen("sample.raw","r");
    fp=fopen(fname,"w");
    fseek(fp_raw,(posn-bestfstart)/FSIZE*80*2,SEEK_SET);
    fread(s,bestfsize/FSIZE*80,2,fp_raw);
    fwrite(s,bestfsize/FSIZE*80,2,fp); fclose(fp); fclose(fp_raw);
    
    posn+=bestfend;
  }
  printf("done! total segments=%i\n",chunk);
}
