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

  function: lookup data; generated by lookups.pl; edit there
  last mod: $Id$

 ********************************************************************/

#ifndef _V_LOOKUP_DATA_H_

#define FROMdB_LOOKUP_SZ 35
#define FROMdB2_LOOKUP_SZ 32
#define FROMdB_SHIFT 5
#define FROMdB2_SHIFT 3
#define FROMdB2_MASK 31

static FIXP FROMdB_LOOKUP[FROMdB_LOOKUP_SZ]={
   TO_FIXP(30,              1),TO_FIXP(30,   0.6309573445),TO_FIXP(30,   0.3981071706),TO_FIXP(30,   0.2511886432),
   TO_FIXP(30,   0.1584893192),TO_FIXP(30,            0.1),TO_FIXP(30,  0.06309573445),TO_FIXP(30,  0.03981071706),
   TO_FIXP(30,  0.02511886432),TO_FIXP(30,  0.01584893192),TO_FIXP(30,           0.01),TO_FIXP(30, 0.006309573445),
   TO_FIXP(30, 0.003981071706),TO_FIXP(30, 0.002511886432),TO_FIXP(30, 0.001584893192),TO_FIXP(30,          0.001),
   TO_FIXP(30,0.0006309573445),TO_FIXP(30,0.0003981071706),TO_FIXP(30,0.0002511886432),TO_FIXP(30,0.0001584893192),
   TO_FIXP(30,         0.0001),TO_FIXP(30,6.309573445e-05),TO_FIXP(30,3.981071706e-05),TO_FIXP(30,2.511886432e-05),
   TO_FIXP(30,1.584893192e-05),TO_FIXP(30,          1e-05),TO_FIXP(30,6.309573445e-06),TO_FIXP(30,3.981071706e-06),
   TO_FIXP(30,2.511886432e-06),TO_FIXP(30,1.584893192e-06),TO_FIXP(30,          1e-06),TO_FIXP(30,6.309573445e-07),
   TO_FIXP(30,3.981071706e-07),TO_FIXP(30,2.511886432e-07),TO_FIXP(30,1.584893192e-07),
};

static FIXP FROMdB2_LOOKUP[FROMdB2_LOOKUP_SZ]={
   TO_FIXP(30,   0.9928302478),TO_FIXP(30,   0.9786445908),TO_FIXP(30,   0.9646616199),TO_FIXP(30,   0.9508784391),
   TO_FIXP(30,   0.9372921937),TO_FIXP(30,     0.92390007),TO_FIXP(30,   0.9106992942),TO_FIXP(30,   0.8976871324),
   TO_FIXP(30,   0.8848608897),TO_FIXP(30,   0.8722179097),TO_FIXP(30,   0.8597555737),TO_FIXP(30,   0.8474713009),
   TO_FIXP(30,    0.835362547),TO_FIXP(30,   0.8234268041),TO_FIXP(30,   0.8116616003),TO_FIXP(30,   0.8000644989),
   TO_FIXP(30,   0.7886330981),TO_FIXP(30,   0.7773650302),TO_FIXP(30,   0.7662579617),TO_FIXP(30,    0.755309592),
   TO_FIXP(30,   0.7445176537),TO_FIXP(30,   0.7338799116),TO_FIXP(30,   0.7233941627),TO_FIXP(30,   0.7130582353),
   TO_FIXP(30,   0.7028699885),TO_FIXP(30,   0.6928273125),TO_FIXP(30,   0.6829281272),TO_FIXP(30,   0.6731703824),
   TO_FIXP(30,   0.6635520573),TO_FIXP(30,   0.6540711597),TO_FIXP(30,   0.6447257262),TO_FIXP(30,   0.6355138211),
};

#define INVSQ_LOOKUP_I_SHIFT 10
#define INVSQ_LOOKUP_I_MASK 1023
static long INVSQ_LOOKUP_I[64+1]={
	   92682,   91966,   91267,   90583,
	   89915,   89261,   88621,   87995,
	   87381,   86781,   86192,   85616,
	   85051,   84497,   83953,   83420,
	   82897,   82384,   81880,   81385,
	   80899,   80422,   79953,   79492,
	   79039,   78594,   78156,   77726,
	   77302,   76885,   76475,   76072,
	   75674,   75283,   74898,   74519,
	   74146,   73778,   73415,   73058,
	   72706,   72359,   72016,   71679,
	   71347,   71019,   70695,   70376,
	   70061,   69750,   69444,   69141,
	   68842,   68548,   68256,   67969,
	   67685,   67405,   67128,   66855,
	   66585,   66318,   66054,   65794,
	   65536,
};

#define COS_LOOKUP_I_SHIFT 9
#define COS_LOOKUP_I_MASK 511
#define COS_LOOKUP_I_SZ 128
static long COS_LOOKUP_I[COS_LOOKUP_I_SZ+1]={
	   16384,   16379,   16364,   16340,
	   16305,   16261,   16207,   16143,
	   16069,   15986,   15893,   15791,
	   15679,   15557,   15426,   15286,
	   15137,   14978,   14811,   14635,
	   14449,   14256,   14053,   13842,
	   13623,   13395,   13160,   12916,
	   12665,   12406,   12140,   11866,
	   11585,   11297,   11003,   10702,
	   10394,   10080,    9760,    9434,
	    9102,    8765,    8423,    8076,
	    7723,    7366,    7005,    6639,
	    6270,    5897,    5520,    5139,
	    4756,    4370,    3981,    3590,
	    3196,    2801,    2404,    2006,
	    1606,    1205,     804,     402,
	       0,    -401,    -803,   -1204,
	   -1605,   -2005,   -2403,   -2800,
	   -3195,   -3589,   -3980,   -4369,
	   -4755,   -5138,   -5519,   -5896,
	   -6269,   -6638,   -7004,   -7365,
	   -7722,   -8075,   -8422,   -8764,
	   -9101,   -9433,   -9759,  -10079,
	  -10393,  -10701,  -11002,  -11296,
	  -11584,  -11865,  -12139,  -12405,
	  -12664,  -12915,  -13159,  -13394,
	  -13622,  -13841,  -14052,  -14255,
	  -14448,  -14634,  -14810,  -14977,
	  -15136,  -15285,  -15425,  -15556,
	  -15678,  -15790,  -15892,  -15985,
	  -16068,  -16142,  -16206,  -16260,
	  -16304,  -16339,  -16363,  -16378,
	  -16383,
};

#endif
