// generate feature data from input raw audio file
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>
#include <strings.h>

#define FFT 512
#define FF2 257 // at 4000Hz, gives 4000/257=15.5Hz resolution.
#define N 80 // 10ms step size (100fps)
#define CHANS 16  // number of filterbanks
#define CEPS 9

#define NW       200  // analysis window size  (25ms)
#define CUBIC 1
#define EMPH 1.0

#define TWO_PI     6.283185307

float formant[CHANS]={150,250,350,450,570,700,840,1000,1170,
                     1370,1600,1850,2150,2500,2900,3400}; //Bark centers (Hz)

//---------------------------------------------------------------------------
// set up standard Bark-scale triangular filterbanks...
void setup_wts(float wts[CHANS][FF2],float iwts[CHANS][FF2])
{
  int i,n,k;
  float sr=8000;
  float minfrq=0;
  float maxfrq=4000.0; 
  float fftfrqs[FFT];
  float binfrqs[CHANS+3];
  float fs[4],fs2[4];
  float loslope[FFT],hislope[FFT];
  float ww[FF2][FF2];
  float mean,sum[FF2];
  float f0;
  
  // get center freqs of each FFT bin... 
  for (i=0; i<FFT; i++) fftfrqs[i]=(float)i*(float)sr/FFT;
 
  binfrqs[1]=50; // lower 50Hz edge of bottom 150Hz Bark bin
  for (i=2; i<=CHANS+1; i++) {
    binfrqs[i]=formant[i-2];
  }
  binfrqs[CHANS+2]=4000; // arbitrary cut-off
  // using 1:CHANS (as in octave) here....
  for (i=1; i<=CHANS; i++) {
    // fs = binfrqs(i+[0 1 2]) - so fs is binfrqs(i+0,i+1,i+2)
    fs[1]=binfrqs[i]; fs[2]=binfrqs[i+1]; fs[3]=binfrqs[i+2];
    // scale by width
    for (n=1; n<=3; n++) fs2[n]=fs[2]+1.0*(fs[n]-fs[2]);
    //bcopy(fs2,fs,4*4); // safety!
    // lower and upper slopes for all bins
    for (n=0; n<FFT; n++) loslope[n]=(fftfrqs[n] - fs[1])/(fs[2] - fs[1]);
    for (n=0; n<FFT; n++) hislope[n]= (fs[3] - fftfrqs[n])/(fs[3] - fs[2]);
    // .. then intersect them with each other and zero
    for (n=0; n<FFT; n++) {
     if (loslope[n]<=hislope[n] && loslope[n]>0) wts[i-1][n]=loslope[n];
     else if (hislope[n]<loslope[n] && hislope[n]>0) wts[i-1][n]=hislope[n];
     else wts[i-1][n]=0;
    }  
  }
  // now convert wts to iwts......
  // ww = wts'*wts;
  for (i=0; i<FF2; i++) for (n=0; n<FF2; n++)
    {ww[i][n]=0; for (k=0; k<CHANS; k++) ww[i][n]+=wts[k][i]*wts[k][n];}
  
  // mean of diag of www...
  mean=0; for (i=0; i<FF2; i++) mean+=ww[i][i]; mean/=FF2; mean/=100.0;
  for (i=0; i<FF2; i++) {sum[i]=0; for (n=0; n<FF2; n++) sum[i]+=ww[i][n];}
  for (i=0; i<FF2; i++) if (sum[i]<mean) sum[i]=mean;
  for (i=0; i<CHANS; i++)
     for (n=0; n<FF2; n++) iwts[i][n]=wts[i][n]/sum[n];     
}

//---------------------------------------------------------------------------

main(int argc, char *argv[])
{
  FILE *fp, *fp_cep;
  char *f_raw, *f_cep;
  float f[FFT],f2[FFT],window[FFT],o[FFT];
  int fi[FF2],f2i[FF2];
  short s[FFT];
  fftwf_complex out[FFT];
  fftwf_plan fft,ifft;
  int i,j,m,n,posn=0,count=0,a;
 
  float mel[CHANS],mel2[CHANS],top,tot,e,e2;
  float wts[CHANS][FF2], iwts[CHANS][FF2], spec[FF2];
  float dctm[CEPS][CHANS],idctm[CEPS][CHANS],cep[CEPS],lastcep[CEPS];
  float prev=0,x,y,prevx=0,prevy=0;
  float last[CHANS];
  int d,best,c,cepi[CEPS];
 
  if (argc!=3) {printf("usage: features <input audio> <features file>\n"); exit(0);}
  for (i=0; i<CEPS; i++) lastcep[i]=0;
  for (i=0; i<CHANS; i++) last[i]=0;
 
  setup_wts(wts,iwts);
   
  // set up mdct arrays...
  for (i=0; i<CEPS; i++) {
    for (j=0,x=1.0; j<CHANS; j++,x+=2.0) {
         dctm[i][j]=cos((float)i*x/CHANS*M_PI/2.0)*sqrtf(2.0/CHANS);
	 idctm[i][j]=dctm[i][j];
	 if (i==0) idctm[i][j]/=2.0;
    }
  }
  
  fft = fftwf_plan_dft_r2c_1d(FFT, f, out, FFTW_PATIENT);
  ifft = fftwf_plan_dft_c2r_1d(FFT, out, f, FFTW_PATIENT);
  
  for(i=0; i<FFT; i++) window[i] = 0.0;
  for(i=FFT/2-NW/2,j=0; i<FFT/2+NW/2; i++,j++) window[i]=0.5-0.5*cos(TWO_PI*j/(NW-1));
  
  for (i=0; i<FFT; i++) f2[i]=0.0;
  for (i=0; i<FFT; i++) s[i]=0;
  
  fp=fopen(argv[1],"r");
  fp_cep=fopen(argv[2],"w"); 
   
  while(!feof(fp) ) {
    fread(s,N,2,fp);
    for (i=FFT-N,j=0; i<FFT; i++,j++) {
      x=(float)s[j]; 
      f2[i]=x-prev*EMPH; prev=x;     
    }
    for (i=0; i<FFT; i++) f[i]=f2[i];
    // remove any DC level....
    x=0; for (i=0; i<FFT; i++) x+=f[i]; x/=FFT;
    for (i=0; i<FFT; i++) f[i]-=x;
    for (i=0; i<FFT; i++) f[i]*=window[i];
   
    fftwf_execute(fft);
    for (i=0; i<FF2; i++) f[i]=(out[i][0]*out[i][0]+out[i][1]*out[i][1]);
  
    // now estimate the 16 bark bins...
    for (n=0; n<CHANS; n++) {
       tot=0.0; for (i=0; i<FF2; i++) tot+=(f[i])*wts[n][i];
       mel[n]=tot;  mel[n]=sqrtf(mel[n]);
    }

    for (n=0; n<CHANS; n++) {  
      if (CUBIC) mel[n]=powf(mel[n],0.333333); else mel[n]=logf(mel[n]);     
    }

    // smooth bins?
    for (n=0; n<CHANS; n++) mel[n]=(mel[n]+last[n])/2;
  
    // calculate  ceps....
    for (i=0; i<CEPS; i++) {
      cep[i]=0; for (n=0; n<CHANS; n++) cep[i]+=mel[n]*dctm[i][n];
    }
    // lifter the ceps....?
    
    for (i=0; i<CEPS; i++) cep[i]=cep[i]*10.0;
    cep[0]=cep[0]/3.0;

    for (i=0; i<CEPS; i++) cepi[i]=cep[i]; // integer
    fwrite(cepi,CEPS,4,fp_cep);  
    
    //for (i=0; i<CEPS; i++) printf("%i,",cepi[i]); printf("\n");
    
    for (i=0; i<CHANS; i++) last[i]=mel[i];
    count++;
 
    for (i=0; i<FFT-N; i++) f2[i]=f2[i+N]; // shift samples down...
    posn++; if (posn%10000==0) fprintf(stderr,"at %i\n",posn);
  }
  fclose(fp); fclose(fp_cep);
  printf("frames=%i, output=%i\n",posn,count);
 
}
