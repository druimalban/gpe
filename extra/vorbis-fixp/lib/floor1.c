/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: floor backend 1 implementation
 last mod: $Id$

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ogg.h"
#include "vorbis_codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "scales.h"

#define floor1_rangedB 140 /* floor 1 fixed at -140dB to 0dB range */

typedef struct {
  int sorted_index[VIF_POSIT+2];
  int forward_index[VIF_POSIT+2];
  int reverse_index[VIF_POSIT+2];
  
  int hineighbor[VIF_POSIT];
  int loneighbor[VIF_POSIT];
  int posts;

  int n;
  int quant_q;
  vorbis_info_floor1 *vi;

  long phrasebits;
  long postbits;
  long frames;
} vorbis_look_floor1;

typedef struct lsfit_acc{
  long x0;
  long x1;

  long xa;
  long ya;
  long x2a;
  long y2a;
  long xya; 
  long n;
  long an;
  long un;
  long edgey0;
  long edgey1;
} lsfit_acc;

/***********************************************/
 
static void floor1_free_info(vorbis_info_floor *i){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

static void floor1_free_look(vorbis_look_floor *i){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  if(look){
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static vorbis_info_floor *floor1_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=vi->codec_setup;
  int j,k,count=0,maxclass=-1,rangebits;

  vorbis_info_floor1 *info=_ogg_calloc(1,sizeof(*info));
  /* read partitions */
  info->partitions=oggpack_read(opb,5); /* only 0 to 31 legal */
  for(j=0;j<info->partitions;j++){
    info->partitionclass[j]=oggpack_read(opb,4); /* only 0 to 15 legal */
    if(maxclass<info->partitionclass[j])maxclass=info->partitionclass[j];
  }

  /* read partition classes */
  for(j=0;j<maxclass+1;j++){
    info->class_dim[j]=oggpack_read(opb,3)+1; /* 1 to 8 */
    info->class_subs[j]=oggpack_read(opb,2); /* 0,1,2,3 bits */
    if(info->class_subs[j]<0)
      goto err_out;
    if(info->class_subs[j])info->class_book[j]=oggpack_read(opb,8);
    if(info->class_book[j]<0 || info->class_book[j]>=ci->books)
      goto err_out;
    for(k=0;k<(1<<info->class_subs[j]);k++){
      info->class_subbook[j][k]=oggpack_read(opb,8)-1;
      if(info->class_subbook[j][k]<-1 || info->class_subbook[j][k]>=ci->books)
	goto err_out;
    }
  }

  /* read the post list */
  info->mult=oggpack_read(opb,2)+1;     /* only 1,2,3,4 legal now */ 
  rangebits=oggpack_read(opb,4);

  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class_dim[info->partitionclass[j]]; 
    for(;k<count;k++){
      int t=info->postlist[k+2]=oggpack_read(opb,rangebits);
      if(t<0 || t>=(1<<rangebits))
	goto err_out;
    }
  }
  info->postlist[0]=0;
  info->postlist[1]=1<<rangebits;

  return(info);
  
 err_out:
  floor1_free_info(info);
  return(NULL);
}

static int icomp(const void *a,const void *b){
  return(**(int **)a-**(int **)b);
}

static vorbis_look_floor *floor1_look(vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *in){

  int *sortpointer[VIF_POSIT+2];
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)in;
  vorbis_look_floor1 *look=_ogg_calloc(1,sizeof(*look));
  int i,j,n=0;

  look->vi=info;
  look->n=info->postlist[1];
 
  /* we drop each position value in-between already decoded values,
     and use linear interpolation to predict each new value past the
     edges.  The positions are read in the order of the position
     list... we precompute the bounding positions in the lookup.  Of
     course, the neighbors can change (if a position is declined), but
     this is an initial mapping */

  for(i=0;i<info->partitions;i++)n+=info->class_dim[info->partitionclass[i]];
  n+=2;
  look->posts=n;

  /* also store a sorted position index */
  for(i=0;i<n;i++)sortpointer[i]=info->postlist+i;
  qsort(sortpointer,n,sizeof(*sortpointer),icomp);

  /* points from sort order back to range number */
  for(i=0;i<n;i++)look->forward_index[i]=sortpointer[i]-info->postlist;
  /* points from range order to sorted position */
  for(i=0;i<n;i++)look->reverse_index[look->forward_index[i]]=i;
  /* we actually need the post values too */
  for(i=0;i<n;i++)look->sorted_index[i]=info->postlist[look->forward_index[i]];
  
  /* quantize values to multiplier spec */
  switch(info->mult){
  case 1: /* 1024 -> 256 */
    look->quant_q=256;
    break;
  case 2: /* 1024 -> 128 */
    look->quant_q=128;
    break;
  case 3: /* 1024 -> 86 */
    look->quant_q=86;
    break;
  case 4: /* 1024 -> 64 */
    look->quant_q=64;
    break;
  }

  /* discover our neighbors for decode where we don't use fit flags
     (that would push the neighbors outward) */
  for(i=0;i<n-2;i++){
    int lo=0;
    int hi=1;
    int lx=0;
    int hx=look->n;
    int currentx=info->postlist[i+2];
    for(j=0;j<i+2;j++){
      int x=info->postlist[j];
      if(x>lx && x<currentx){
	lo=j;
	lx=x;
      }
      if(x<hx && x>currentx){
	hi=j;
	hx=x;
      }
    }
    look->loneighbor[i]=lo;
    look->hineighbor[i]=hi;
  }

  return(look);
}

static int render_point(int x0,int x1,int y0,int y1,int x){
  y0&=0x7fff; /* mask off flag */
  y1&=0x7fff;
    
  {
    int dy=y1-y0;
    int adx=x1-x0;
    int ady=abs(dy);
    int err=ady*(x-x0);
    
    int off=err/adx;
    if(dy<0)return(y0-off);
    return(y0+off);
  }
}

static FIXP FLOOR_fromdB_LOOKUP[256]={
	TO_FIXP(30,1.0649863e-07F), TO_FIXP(30,1.1341951e-07F), 
	TO_FIXP(30,1.2079015e-07F), TO_FIXP(30,1.2863978e-07F),
	TO_FIXP(30,1.3699951e-07F), TO_FIXP(30,1.4590251e-07F), 
	TO_FIXP(30,1.5538408e-07F), TO_FIXP(30,1.6548181e-07F),
	TO_FIXP(30,1.7623575e-07F), TO_FIXP(30,1.8768855e-07F), 
	TO_FIXP(30,1.9988561e-07F), TO_FIXP(30,2.128753e-07F),
	TO_FIXP(30,2.2670913e-07F), TO_FIXP(30,2.4144197e-07F), 
	TO_FIXP(30,2.5713223e-07F), TO_FIXP(30,2.7384213e-07F),
	TO_FIXP(30,2.9163793e-07F), TO_FIXP(30,3.1059021e-07F), 
	TO_FIXP(30,3.3077411e-07F), TO_FIXP(30,3.5226968e-07F),
	TO_FIXP(30,3.7516214e-07F), TO_FIXP(30,3.9954229e-07F), 
	TO_FIXP(30,4.2550680e-07F), TO_FIXP(30,4.5315863e-07F),
	TO_FIXP(30,4.8260743e-07F), TO_FIXP(30,5.1396998e-07F), 
	TO_FIXP(30,5.4737065e-07F), TO_FIXP(30,5.8294187e-07F),
	TO_FIXP(30,6.2082472e-07F), TO_FIXP(30,6.6116941e-07F), 
	TO_FIXP(30,7.0413592e-07F), TO_FIXP(30,7.4989464e-07F),
	TO_FIXP(30,7.9862701e-07F), TO_FIXP(30,8.5052630e-07F), 
	TO_FIXP(30,9.0579828e-07F), TO_FIXP(30,9.6466216e-07F),
	TO_FIXP(30,1.0273513e-06F), TO_FIXP(30,1.0941144e-06F), 
	TO_FIXP(30,1.1652161e-06F), TO_FIXP(30,1.2409384e-06F),
	TO_FIXP(30,1.3215816e-06F), TO_FIXP(30,1.4074654e-06F), 
	TO_FIXP(30,1.4989305e-06F), TO_FIXP(30,1.5963394e-06F),
	TO_FIXP(30,1.7000785e-06F), TO_FIXP(30,1.8105592e-06F), 
	TO_FIXP(30,1.9282195e-06F), TO_FIXP(30,2.0535261e-06F),
	TO_FIXP(30,2.1869758e-06F), TO_FIXP(30,2.3290978e-06F), 
	TO_FIXP(30,2.4804557e-06F), TO_FIXP(30,2.6416497e-06F),
	TO_FIXP(30,2.8133190e-06F), TO_FIXP(30,2.9961443e-06F), 
	TO_FIXP(30,3.1908506e-06F), TO_FIXP(30,3.3982101e-06F),
	TO_FIXP(30,3.6190449e-06F), TO_FIXP(30,3.8542308e-06F), 
	TO_FIXP(30,4.1047004e-06F), TO_FIXP(30,4.3714470e-06F),
	TO_FIXP(30,4.6555282e-06F), TO_FIXP(30,4.9580707e-06F), 
	TO_FIXP(30,5.2802740e-06F), TO_FIXP(30,5.6234160e-06F),
	TO_FIXP(30,5.9888572e-06F), TO_FIXP(30,6.3780469e-06F), 
	TO_FIXP(30,6.7925283e-06F), TO_FIXP(30,7.2339451e-06F),
	TO_FIXP(30,7.7040476e-06F), TO_FIXP(30,8.2047000e-06F), 
	TO_FIXP(30,8.7378876e-06F), TO_FIXP(30,9.3057248e-06F),
	TO_FIXP(30,9.9104632e-06F), TO_FIXP(30,1.0554501e-05F), 
	TO_FIXP(30,1.1240392e-05F), TO_FIXP(30,1.1970856e-05F),
	TO_FIXP(30,1.2748789e-05F), TO_FIXP(30,1.3577278e-05F), 
	TO_FIXP(30,1.4459606e-05F), TO_FIXP(30,1.5399272e-05F),
	TO_FIXP(30,1.6400004e-05F), TO_FIXP(30,1.7465768e-05F), 
	TO_FIXP(30,1.8600792e-05F), TO_FIXP(30,1.9809576e-05F),
	TO_FIXP(30,2.1096914e-05F), TO_FIXP(30,2.2467911e-05F), 
	TO_FIXP(30,2.3928002e-05F), TO_FIXP(30,2.5482978e-05F),
	TO_FIXP(30,2.7139006e-05F), TO_FIXP(30,2.8902651e-05F), 
	TO_FIXP(30,3.0780908e-05F), TO_FIXP(30,3.2781225e-05F),
	TO_FIXP(30,3.4911534e-05F), TO_FIXP(30,3.7180282e-05F), 
	TO_FIXP(30,3.9596466e-05F), TO_FIXP(30,4.2169667e-05F),
	TO_FIXP(30,4.4910090e-05F), TO_FIXP(30,4.7828601e-05F), 
	TO_FIXP(30,5.0936773e-05F), TO_FIXP(30,5.4246931e-05F),
	TO_FIXP(30,5.7772202e-05F), TO_FIXP(30,6.1526565e-05F), 
	TO_FIXP(30,6.5524908e-05F), TO_FIXP(30,6.9783085e-05F),
	TO_FIXP(30,7.4317983e-05F), TO_FIXP(30,7.9147585e-05F), 
	TO_FIXP(30,8.4291040e-05F), TO_FIXP(30,8.9768747e-05F),
	TO_FIXP(30,9.5602426e-05F), TO_FIXP(30,0.00010181521F), 
	TO_FIXP(30,0.00010843174F), TO_FIXP(30,0.00011547824F),
	TO_FIXP(30,0.00012298267F), TO_FIXP(30,0.00013097477F), 
	TO_FIXP(30,0.00013948625F), TO_FIXP(30,0.00014855085F),
	TO_FIXP(30,0.00015820453F), TO_FIXP(30,0.00016848555F), 
	TO_FIXP(30,0.00017943469F), TO_FIXP(30,0.00019109536F),
	TO_FIXP(30,0.00020351382F), TO_FIXP(30,0.00021673929F), 
	TO_FIXP(30,0.00023082423F), TO_FIXP(30,0.00024582449F),
	TO_FIXP(30,0.00026179955F), TO_FIXP(30,0.00027881276F), 
	TO_FIXP(30,0.00029693158F), TO_FIXP(30,0.00031622787F),
	TO_FIXP(30,0.00033677814F), TO_FIXP(30,0.00035866388F), 
	TO_FIXP(30,0.00038197188F), TO_FIXP(30,0.00040679456F),
	TO_FIXP(30,0.00043323036F), TO_FIXP(30,0.00046138411F), 
	TO_FIXP(30,0.00049136745F), TO_FIXP(30,0.00052329927F),
	TO_FIXP(30,0.00055730621F), TO_FIXP(30,0.00059352311F), 
	TO_FIXP(30,0.00063209358F), TO_FIXP(30,0.00067317058F),
	TO_FIXP(30,0.00071691700F), TO_FIXP(30,0.00076350630F), 
	TO_FIXP(30,0.00081312324F), TO_FIXP(30,0.00086596457F),
	TO_FIXP(30,0.00092223983F), TO_FIXP(30,0.00098217216F), 
	TO_FIXP(30,0.0010459992F), TO_FIXP(30,0.0011139742F),
	TO_FIXP(30,0.0011863665F), TO_FIXP(30,0.0012634633F),
	TO_FIXP(30,0.0013455702F), TO_FIXP(30,0.0014330129F),
	TO_FIXP(30,0.0015261382F), TO_FIXP(30,0.0016253153F),
	TO_FIXP(30,0.0017309374F), TO_FIXP(30,0.0018434235F),
	TO_FIXP(30,0.0019632195F), TO_FIXP(30,0.0020908006F),
	TO_FIXP(30,0.0022266726F), TO_FIXP(30,0.0023713743F),
	TO_FIXP(30,0.0025254795F), TO_FIXP(30,0.0026895994F),
	TO_FIXP(30,0.0028643847F), TO_FIXP(30,0.0030505286F),
	TO_FIXP(30,0.0032487691F), TO_FIXP(30,0.0034598925F),
	TO_FIXP(30,0.0036847358F), TO_FIXP(30,0.0039241906F),
	TO_FIXP(30,0.0041792066F), TO_FIXP(30,0.0044507950F),
	TO_FIXP(30,0.0047400328F), TO_FIXP(30,0.0050480668F),
	TO_FIXP(30,0.0053761186F), TO_FIXP(30,0.0057254891F),
	TO_FIXP(30,0.0060975636F), TO_FIXP(30,0.0064938176F),
	TO_FIXP(30,0.0069158225F), TO_FIXP(30,0.0073652516F),
	TO_FIXP(30,0.0078438871F), TO_FIXP(30,0.0083536271F),
	TO_FIXP(30,0.0088964928F), TO_FIXP(30,0.009474637F),
	TO_FIXP(30,0.010090352F), TO_FIXP(30,0.010746080F),
	TO_FIXP(30,0.011444421F), TO_FIXP(30,0.012188144F),
	TO_FIXP(30,0.012980198F), TO_FIXP(30,0.013823725F),
	TO_FIXP(30,0.014722068F), TO_FIXP(30,0.015678791F),
	TO_FIXP(30,0.016697687F), TO_FIXP(30,0.017782797F),
	TO_FIXP(30,0.018938423F), TO_FIXP(30,0.020169149F),
	TO_FIXP(30,0.021479854F), TO_FIXP(30,0.022875735F),
	TO_FIXP(30,0.024362330F), TO_FIXP(30,0.025945531F),
	TO_FIXP(30,0.027631618F), TO_FIXP(30,0.029427276F),
	TO_FIXP(30,0.031339626F), TO_FIXP(30,0.033376252F),
	TO_FIXP(30,0.035545228F), TO_FIXP(30,0.037855157F),
	TO_FIXP(30,0.040315199F), TO_FIXP(30,0.042935108F),
	TO_FIXP(30,0.045725273F), TO_FIXP(30,0.048696758F),
	TO_FIXP(30,0.051861348F), TO_FIXP(30,0.055231591F),
	TO_FIXP(30,0.058820850F), TO_FIXP(30,0.062643361F),
	TO_FIXP(30,0.066714279F), TO_FIXP(30,0.071049749F),
	TO_FIXP(30,0.075666962F), TO_FIXP(30,0.080584227F),
	TO_FIXP(30,0.085821044F), TO_FIXP(30,0.091398179F),
	TO_FIXP(30,0.097337747F), TO_FIXP(30,0.10366330F),
	TO_FIXP(30,0.11039993F), TO_FIXP(30,0.11757434F), 
	TO_FIXP(30,0.12521498F), TO_FIXP(30,0.13335215F),
	TO_FIXP(30,0.14201813F), TO_FIXP(30,0.15124727F),
	TO_FIXP(30,0.16107617F), TO_FIXP(30,0.17154380F),
	TO_FIXP(30,0.18269168F), TO_FIXP(30,0.19456402F),
	TO_FIXP(30,0.20720788F), TO_FIXP(30,0.22067342F),
	TO_FIXP(30,0.23501402F), TO_FIXP(30,0.25028656F),
	TO_FIXP(30,0.26655159F), TO_FIXP(30,0.28387361F),
	TO_FIXP(30,0.30232132F), TO_FIXP(30,0.32196786F),
	TO_FIXP(30,0.34289114F), TO_FIXP(30,0.36517414F),
	TO_FIXP(30,0.38890521F), TO_FIXP(30,0.41417847F),
	TO_FIXP(30,0.44109412F), TO_FIXP(30,0.46975890F),
	TO_FIXP(30,0.50028648F), TO_FIXP(30,0.53279791F),
	TO_FIXP(30,0.56742212F), TO_FIXP(30,0.60429640F),
	TO_FIXP(30,0.64356699F), TO_FIXP(30,0.68538959F),
	TO_FIXP(30,0.72993007F), TO_FIXP(30,0.77736504F),
	TO_FIXP(30,0.82788260F), TO_FIXP(30,0.88168307F),
	TO_FIXP(30,0.9389798F), TO_FIXP(30,1.F),
};

static void render_line(int x0,int x1,int y0,int y1,FIXP *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;

  ady-=abs(base*adx);

  /*
   * FIXP note: d[] (x.16) * FLOOR_fromdB_LOOKUP[] (x.30) = (x.46).
   * We want to scale it back to FIXP_FRACBITS.
   */
  d[x]=MUL(46-FIXP_FRACBITS,d[x],FLOOR_fromdB_LOOKUP[y]);
  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]=MUL(46-FIXP_FRACBITS,d[x],FLOOR_fromdB_LOOKUP[y]);
  }
}

static void *floor1_inverse1(vorbis_block *vb,vorbis_look_floor *in){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;
  codec_setup_info   *ci=vb->vd->vi->codec_setup;
  
  int i,j,k;
  codebook *books=ci->fullbooks;   

  /* unpack wrapped/predicted values from stream */
  if(oggpack_read(&vb->opb,1)==1){
    int *fit_value=_vorbis_block_alloc(vb,(look->posts)*sizeof(*fit_value));

    fit_value[0]=oggpack_read(&vb->opb,ilog(look->quant_q-1));
    fit_value[1]=oggpack_read(&vb->opb,ilog(look->quant_q-1));

    /* partition by partition */
    /* partition by partition */
    for(i=0,j=2;i<info->partitions;i++){
      int class=info->partitionclass[i];
      int cdim=info->class_dim[class];
      int csubbits=info->class_subs[class];
      int csub=1<<csubbits;
      int cval=0;

      /* decode the partition's first stage cascade value */
      if(csubbits){
	cval=vorbis_book_decode(books+info->class_book[class],&vb->opb);

	if(cval==-1)goto eop;
      }

      for(k=0;k<cdim;k++){
	int book=info->class_subbook[class][cval&(csub-1)];
	cval>>=csubbits;
	if(book>=0){
	  if((fit_value[j+k]=vorbis_book_decode(books+book,&vb->opb))==-1)
	    goto eop;
	}else{
	  fit_value[j+k]=0;
	}
      }
      j+=cdim;
    }

    /* unwrap positive values and reconsitute via linear interpolation */
    for(i=2;i<look->posts;i++){
      int predicted=render_point(info->postlist[look->loneighbor[i-2]],
				 info->postlist[look->hineighbor[i-2]],
				 fit_value[look->loneighbor[i-2]],
				 fit_value[look->hineighbor[i-2]],
				 info->postlist[i]);
      int hiroom=look->quant_q-predicted;
      int loroom=predicted;
      int room=(hiroom<loroom?hiroom:loroom)<<1;
      int val=fit_value[i];

      if(val){
	if(val>=room){
	  if(hiroom>loroom){
	    val = val-loroom;
	  }else{
	  val = -1-(val-hiroom);
	  }
	}else{
	  if(val&1){
	    val= -((val+1)>>1);
	  }else{
	    val>>=1;
	  }
	}

	fit_value[i]=val+predicted;
	fit_value[look->loneighbor[i-2]]&=0x7fff;
	fit_value[look->hineighbor[i-2]]&=0x7fff;

      }else{
	fit_value[i]=predicted|0x8000;
      }
	
    }

    return(fit_value);
  }
 eop:
  return(NULL);
}

static int floor1_inverse2(vorbis_block *vb,vorbis_look_floor *in,void *memo,
			  FIXP *out){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;

  codec_setup_info   *ci=vb->vd->vi->codec_setup;
  int                  n=ci->blocksizes[vb->mode]/2;
  int j;

  if(memo){
    /* render the lines */
    int *fit_value=(int *)memo;
    int hx=0;
    int lx=0;
    int ly=fit_value[0]*info->mult;
    for(j=1;j<look->posts;j++){
      int current=look->forward_index[j];
      int hy=fit_value[current]&0x7fff;
      if(hy==fit_value[current]){
	
	hy*=info->mult;
	hx=info->postlist[current];
	
	render_line(lx,hx,ly,hy,out);
	
	lx=hx;
	ly=hy;
      }
    }
    for(j=hx;j<n;j++)out[j]*=ly; /* be certain */    
    return(1);
  }
  memset(out,0,sizeof(*out)*n);
  return(0);
}

/* export hooks */
vorbis_func_floor floor1_exportbundle={
  NULL,&floor1_unpack,&floor1_look,NULL,&floor1_free_info,
  &floor1_free_look,NULL,&floor1_inverse1,&floor1_inverse2
};

