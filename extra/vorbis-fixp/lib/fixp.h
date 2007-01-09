/*
 * Fixed point support for the Ogg Vorbis audio decoding library
 *
 * Copyright (C) 2002 by Nicolas Pitre <nico@cam.org>
 *
 * You can use and/or redistribute this code and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation. See the file COPYING for details.
 */

#ifndef FIXP_H
#define FIXP_H


#define FIXED_POINT
#ifdef FIXED_POINT

#define FIXP_FRACBITS	28

typedef int FIXP;

/* 
 * Rounding doesn't seem to make any visible difference on the final output,
 * we therefore neglect it for less CPU usage.
 */
#define TO_FIXP(s, x)	(FIXP)((x) * (1 << (s)))

//#define FIXED_POINT_CHK
#ifndef FIXED_POINT_CHK

#define TO_FIXP_CHK	TO_FIXP

#if defined(__arm__)

/* ARM optimized multiplication with proper rounding */
#define MUL(s, a, b)  ({ \
	register int res, tmp; \
	asm (	"smull	%0, %1, %2, %3\t@ mul 1" \
		: "=&r" (res), "=&r" (tmp) \
		: "%r" (a), "r" (b)); \
	asm (	"movs	%0, %0, lsr %3\t@ mul 2\n\t" \
		"adc	%0, %0, %2, lsl %4" \
		: "=&r" (res) \
		: "0" (res), "r" (tmp), "M" (s), "M"(32 - (s)) \
		: "cc"); \
	res; })

#else

/* Generic portable multiplication, need long long support */
#define MUL(s, a, b)	(FIXP)(((long long)(a) * (b)) >> (s))

#endif

#else  /* FIXED_POINT_CHK */

/* This is for debugging only */

#include <stdio.h>

#define TO_FIXP_CHK(s, x) \
({ \
	if ((x) >= (1 << (31-(s))) || (x) < -(1 << (31-(s)))) \
		fprintf(stderr, "TO_FIXP(%d) ovf: %g [%s:%d]\n", \
			(s), (float)(x), __FUNCTION__, __LINE__); \
	if ((x) < 1.0/(1 << (s)) && (x) > -1.0/(1 << (s)) && (x) != 0.0) \
		fprintf(stderr, "TO_FIXP(%d) udf: %g [%s:%d]\n", \
			(s), (float)(x), __FUNCTION__, __LINE__); \
	TO_FIXP(s, x); \
})

#define MUL(s, a, b) \
({ \
	long long __x = ((long long)(a) * (b)) >> (s); \
	if (__x >= (1ll << 31) || __x < -(1ll << 31)) \
		fprintf(stderr, "MUL(%d) ovf: 0x%08x * 0x%08x (%s:%d)\n", \
			(s), (a), (b), __FUNCTION__, __LINE__); \
	(FIXP)__x; \
})

#endif  /* FIXED_POINT_CHK */

#else  /* FIXED_POINT */

typedef float FIXP;

#define TO_FIXP(s, x)	(x)
#define TO_FIXP_CHK	TO_FIXP
#define MUL(s, a, b)	((a) * (b))

#endif  /* FIXED_POINT */

#endif  /* FIXP_H */
