/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: LSP (also called LSF) conversion routines
  last mod: $Id$

  The LSP generation code is taken (with minimal modification and a
  few bugfixes) from "On the Computation of the LSP Frequencies" by
  Joseph Rothweiler <rothwlr@altavista.net>, available at:
  
  http://www2.xtdl.com/~rothwlr/lsfpaper/lsfpage.html 

 ********************************************************************/

/* Note that the lpc-lsp conversion finds the roots of polynomial with
   an iterative root polisher (CACM algorithm 283).  It *is* possible
   to confuse this algorithm into not converging; that should only
   happen with absurdly closely spaced roots (very sharp peaks in the
   LPC f response) which in turn should be impossible in our use of
   the code.  If this *does* happen anyway, it's a bug in the floor
   finder; find the cause of the confusion (probably a single bin
   spike or accidental near-float-limit resolution problems) and
   correct it. */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "ogg_os_types.h"
#include "lsp.h"
#include "os.h"
#include "misc.h"
#include "lookup.h"
#include "scales.h"


#define INT_LOOKUP

#if defined(FIXED_POINT) || defined(INT_LOOKUP)

#include "lookup.c" /* catch this in the build system; we #include for
                       compilers (like gcc) that can't inline across
                       modules */

static int MLOOP_1[64]={
   0,10,11,11, 12,12,12,12, 13,13,13,13, 13,13,13,13,
  14,14,14,14, 14,14,14,14, 14,14,14,14, 14,14,14,14,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
};

static int MLOOP_2[64]={
  0,4,5,5, 6,6,6,6, 7,7,7,7, 7,7,7,7,
  8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
};

static int MLOOP_3[8]={0,1,2,2,3,3,3,3};


/* side effect: changes *lsp to cosines of lsp */
void vorbis_lsp_to_curve(FIXP *curve,int *map,int n,int ln,FIXP *lsp,int m,
			    FIXP amp,FIXP ampoffset){

  /* 0 <= m < 256 */

  int i;

#ifndef FIXED_POINT
  int ampoffseti=rint(ampoffset*4096.f);
  int ampi=rint(amp*16.f);
  long *ilsp=alloca(m*sizeof(*ilsp));
  for(i=0;i<m;i++)ilsp[i]=vorbis_coslook_i(lsp[i]/M_PI*65536.f+.5f);
#else
  int ampoffseti=ampoffset*4096;
  int ampi=amp;
  long *ilsp=lsp;
  for(i=0;i<m;i++)ilsp[i]=vorbis_coslook_i(MUL(16,lsp[i],(int)(65536.0/M_PI+.5)));
#endif
  int rln = 65536/ln;

  i=0;
  while(i<n){
    int j,k=map[i];
    unsigned long pi=46341; /* 2**-.5 in 0.16 */
    unsigned long qi=46341;
    int qexp=0,shift;
    long wi=vorbis_coslook_i(k*rln);

    qi*=labs(ilsp[0]-wi);
    pi*=labs(ilsp[1]-wi);

    for(j=3;j<m;j+=2){
      if(!(shift=MLOOP_1[(pi|qi)>>25]))
	if(!(shift=MLOOP_2[(pi|qi)>>19]))
	  shift=MLOOP_3[(pi|qi)>>16];
      qi=(qi>>shift)*labs(ilsp[j-1]-wi);
      pi=(pi>>shift)*labs(ilsp[j]-wi);
      qexp+=shift;
    }
    if(!(shift=MLOOP_1[(pi|qi)>>25]))
      if(!(shift=MLOOP_2[(pi|qi)>>19]))
	shift=MLOOP_3[(pi|qi)>>16];

    /* pi,qi normalized collectively, both tracked using qexp */

    if(m&1){
      /* odd order filter; slightly assymetric */
      /* the last coefficient */
      qi=(qi>>shift)*labs(ilsp[j-1]-wi);
      pi=(pi>>shift)<<14;
      qexp+=shift;

      if(!(shift=MLOOP_1[(pi|qi)>>25]))
	if(!(shift=MLOOP_2[(pi|qi)>>19]))
	  shift=MLOOP_3[(pi|qi)>>16];
      
      pi>>=shift;
      qi>>=shift;
      qexp+=shift-14*((m+1)>>1);

      pi=((pi*pi)>>16);
      qi=((qi*qi)>>16);
      qexp=qexp*2+m;

      pi*=(1<<14)-((wi*wi)>>14);
      qi+=pi>>14;

    }else{
      /* even order filter; still symmetric */

      /* p*=p(1-w), q*=q(1+w), let normalization drift because it isn't
	 worth tracking step by step */
      
      pi>>=shift;
      qi>>=shift;
      qexp+=shift-7*m;

      pi=((pi*pi)>>16);
      qi=((qi*qi)>>16);
      qexp=qexp*2+m;
      
      pi*=(1<<14)-wi;
      qi*=(1<<14)+wi;
      qi=(qi+pi)>>14;
      
    }
    

    /* we've let the normalization drift because it wasn't important;
       however, for the lookup, things must be normalized again.  We
       need at most one right shift or a number of left shifts */

    if(qi&0xffff0000){ /* checks for 1.xxxxxxxxxxxxxxxx */
      qi>>=1; qexp++; 
    }else
      while(qi && !(qi&0x8000)){ /* checks for 0.0xxxxxxxxxxxxxxx or less*/
	qi<<=1; qexp--; 
      }

    amp=vorbis_fromdBlook_i(ampi*                     /*  n.4         */
			    vorbis_invsqlook_i(qi,qexp)- 
			                              /*  m.8, m+n<=8 */
			    ampoffseti);              /*  8.12[0]     */

    /*
     * FIXP note: curve[] (x.16) * amp (x.30) = (x.46).
     * We want to scale it back to FIXP_FRACBITS.
     */
    curve[i]=MUL(46-FIXP_FRACBITS,curve[i],amp);
    while(map[++i]==k)curve[i]=MUL(46-FIXP_FRACBITS,curve[i],amp);
  }
}

#else

/* old, nonoptimized but simple version for any poor sap who needs to
   figure out what the hell this code does, or wants the other
   fraction of a dB precision */

/* side effect: changes *lsp to cosines of lsp */
void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,float *lsp,int m,
			    float amp,float ampoffset){
  int i;
  float wdel=M_PI/ln;
  for(i=0;i<m;i++)lsp[i]=2.f*cos(lsp[i]);

  i=0;
  while(i<n){
    int j,k=map[i];
    float p=.5f;
    float q=.5f;
    float w=2.f*cos(wdel*k);
    for(j=1;j<m;j+=2){
      q *= w-lsp[j-1];
      p *= w-lsp[j];
    }
    if(j==m){
      /* odd order filter; slightly assymetric */
      /* the last coefficient */
      q*=w-lsp[j-1];
      p*=p*(4.f-w*w);
      q*=q;
    }else{
      /* even order filter; still symmetric */
      p*=p*(2.f-w);
      q*=q*(2.f+w);
    }

    q=fromdB(amp/sqrt(p+q)-ampoffset);

    curve[i]*=q;
    while(map[++i]==k)curve[i]*=q;
  }
}

#endif
