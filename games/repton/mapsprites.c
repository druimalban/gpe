/*	$Id$	*/

/*
 * Copyright (c) 1996
 *	Sandro Sigala, Brescia, Italy.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Sandro Sigala.
 * 4. Neither the name of the author nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * If you define the macro ORIG_REPTON you will get all the original
 * ugly 4x8 Repton map sprites, else my better 8x8 sprites.
 */

#define b 9
#define B b,b
#define v 10
#define V v,v
#define g 14
#define G g,g
#define _ 0,0

typedef char mapobj[8][8];

static mapobj mapobjs[] = {
#ifndef ORIG_REPTON
	{ /* 1 */
	{0,0,b,v,g,0,0,0},
	{0,b,b,b,b,g,0,0},
	{b,b,b,b,b,b,g,0},
	{b,b,b,b,b,b,v,0},
	{b,b,b,b,b,b,b,0},
	{0,b,b,b,b,b,0,0},
	{0,0,b,b,b,0,0,0},
	{0,0,0,0,0,0,0,0},
	},
#else
	{ /* 1 */
	{_,_,_,_},
	{_,B,V,_},
	{B,B,B,V},
	{B,B,B,V},
	{B,B,B,B},
	{B,B,B,B},
	{B,B,B,B},
	{_,B,B,_},
	},
#endif

#ifndef ORIG_REPTON
	{ /* 2 */
	{0,0,0,g,0,0,0,0},
	{0,0,g,0,g,0,0,0},
	{0,g,0,g,0,g,0,0},
	{g,0,g,0,0,0,g,0},
	{0,g,0,g,0,g,0,0},
	{0,0,g,0,g,0,0,0},
	{0,0,0,g,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	},
#else
	{ /* 2 */
	{_,_,_,G},
	{_,_,G,G},
	{_,G,G,G},
	{G,G,G,G},
	{G,G,G,G},
	{G,G,G,_},
	{G,G,_,_},
	{G,_,_,_},
	},
#endif

	{ /* 3 */
	{_,_,_,_},
	{_,B,_,_},
	{_,_,_,_},
	{B,_,_,_},
	{_,_,B,_},
	{_,_,_,_},
	{B,_,_,_},
	{_,_,_,_},
	},

	{ /* 4 */
	{_,_,_,_},
	{B,_,B,_},
	{_,_,_,_},
	{_,B,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{B,_,_,_},
	{_,_,_,_},
	},

#ifndef ORIG_REPTON
	{ /* 5 */
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{g,g,0,0,0,g,g,0},
	{g,g,b,b,b,g,g,0},
	{g,g,0,0,0,g,g,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	},
#else
	{ /* 5 */
	{_,_,_,_},
	{G,_,G,_},
	{G,B,G,_},
	{G,B,G,_},
	{G,B,G,_},
	{G,_,G,_},
	{_,_,_,_},
	{_,_,_,_},
	},
#endif

	{ /* 6 */
	{_,G,G,_},
	{G,G,G,G},
	{_,G,G,_},
	{G,G,G,G},
	{G,G,G,G},
	{G,G,G,G},
	{_,G,G,_},
	{_,G,G,_},
	},

	{ /* 7 */
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	},

	{ /* 8 */
	{B,B,_,B},
	{_,_,_,_},
	{B,_,B,B},
	{_,_,_,_},
	{B,B,_,B},
	{_,_,_,_},
	{B,_,B,B},
	{_,_,_,_},
	},

	{ /* 9 */
	{G,B,_,B},
	{G,_,_,_},
	{G,_,B,B},
	{G,_,_,_},
	{G,B,_,B},
	{G,_,_,_},
	{G,_,B,B},
	{G,_,_,_},
	},

	{ /* 10 */
	{B,B,_,G},
	{_,_,_,G},
	{B,_,B,G},
	{_,_,_,G},
	{B,B,_,G},
	{_,_,_,G},
	{B,_,B,G},
	{_,_,_,G},
	},

	{ /* 11 */
	{G,G,G,G},
	{_,_,_,_},
	{B,_,B,B},
	{_,_,_,_},
	{B,B,_,B},
	{_,_,_,_},
	{B,_,B,B},
	{_,_,_,_},
	},

	{ /* 12 */
	{B,B,_,B},
	{_,_,_,_},
	{B,_,B,B},
	{_,_,_,_},
	{B,B,_,B},
	{_,_,_,_},
	{B,_,B,B},
	{G,G,G,G},
	},

	{ /* 13 */
	{_,_,_,G},
	{_,_,G,_},
	{_,G,B,B},
	{_,G,_,_},
	{G,B,_,B},
	{G,_,_,_},
	{G,_,B,B},
	{G,_,_,_},
	},

	{ /* 14 */
	{G,_,_,_},
	{_,G,_,_},
	{B,_,G,_},
	{_,_,G,_},
	{B,B,_,G},
	{_,_,_,G},
	{B,_,B,G},
	{_,_,_,G},
	},

	{ /* 15 */
	{G,B,_,B},
	{G,_,_,_},
	{G,_,B,B},
	{G,_,_,_},
	{_,G,_,B},
	{_,G,_,_},
	{_,_,G,B},
	{_,_,_,G},
	},

	{ /* 16 */
	{B,B,_,G},
	{_,_,_,G},
	{B,_,B,G},
	{_,_,_,G},
	{B,B,G,_},
	{_,_,G,_},
	{B,G,_,_},
	{G,_,_,_},
	},

	{ /* 17 */
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	},

	{ /* 18 */
	{_,_,_,B},
	{_,_,B,G},
	{_,B,B,B},
	{_,B,G,B},
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	},

	{ /* 19 */
	{B,_,_,_},
	{B,G,_,_},
	{B,B,B,_},
	{G,B,G,_},
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	},

	{ /* 20 */
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	{_,B,B,B},
	{_,G,B,G},
	{_,_,B,B},
	{_,_,_,B},
	},

	{ /* 21 */
	{B,B,B,B},
	{B,G,B,G},
	{B,B,B,B},
	{G,B,G,B},
	{B,B,B,_},
	{B,G,B,_},
	{B,B,_,_},
	{G,_,_,_},
	},

	{ /* 22 */
	{G,_,G,_},
	{G,_,G,_},
	{G,_,G,_},
	{_,_,_,_},
	{_,_,_,_},
	{G,_,G,_},
	{G,_,G,_},
	{G,_,G,_},
	},

#ifndef ORIG_REPTON
	{
	{b,b,b,b,b,b,b,b},
	{b,v,b,v,b,v,b,b},
	{g,g,v,b,v,b,v,b},
	{b,v,b,v,b,v,b,b},
	{b,b,v,b,v,b,v,b},
	{g,g,b,v,b,v,b,b},
	{b,b,v,b,v,b,v,b},
	{b,b,b,b,b,b,b,b},
	},
#else
	{ /* 23 */
	{B,B,B,B},
	{B,V,B,B},
	{G,B,V,B},
	{B,V,B,B},
	{B,B,V,B},
	{G,V,B,B},
	{B,B,V,B},
	{B,B,B,B},
	},
#endif

#ifndef ORIG_REPTON
	{ /* 24 */
	{g,g,g,g,g,g,g,0},
	{g,0,g,0,g,0,g,0},
	{g,0,g,0,g,0,b,0},
	{g,0,g,0,g,b,b,0},
	{g,0,g,0,g,b,g,0},
	{g,0,g,0,g,0,g,0},
	{g,0,g,0,g,0,g,0},
	{g,g,g,g,g,g,g,0},
	},
#else
	{ /* 24 */
	{G,G,G,_},
	{G,_,G,_},
	{G,_,G,_},
	{G,_,G,_},
	{G,_,_,B},
	{G,_,B,_},
	{G,_,G,_},
	{G,G,G,_},
	},
#endif

#ifndef ORIG_REPTON
	{ /* 25 */
	{0,0,0,g,g,0,0,0},
	{0,0,g,g,g,g,0,0},
	{0,g,g,g,g,g,g,0},
	{g,g,g,g,g,g,g,g},
	{g,g,g,g,g,g,g,g},
	{g,g,g,g,g,g,g,g},
	{0,g,g,g,g,g,g,0},
	{0,0,g,g,g,g,0,0},
	},
#else
	{ /* 25 */
	{_,G,G,_},
	{_,G,G,_},
	{_,G,G,_},
	{G,G,G,G},
	{G,G,G,G},
	{G,G,G,G},
	{G,G,G,G},
	{_,G,G,_},
	},
#endif

#ifndef ORIG_REPTON
	{ /* 26 */
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,g,g,0},
	{g,g,g,g,g,b,b,g},
	{g,g,0,0,0,g,g,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	},
#else
	{ /* 26 */
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,_,G,G},
	{G,G,_,G},
	{G,_,G,G},
	{_,_,_,_},
	{_,_,_,_},
	},
#endif

	{ /* 27 */
	{B,_,_,B},
	{_,B,B,_},
	{_,B,_,_},
	{B,B,G,_},
	{_,V,B,B},
	{_,G,V,_},
	{_,B,B,_},
	{B,_,_,B},
	},

	{ /* 28 */
	{V,_,_,_},
	{_,V,_,_},
	{_,B,B,_},
	{B,B,B,B},
	{B,B,B,B},
	{B,B,B,B},
	{B,B,B,B},
	{_,B,B,_},
	},

	{ /* 29 */
	{_,_,_,_},
	{G,G,G,_},
	{G,B,G,_},
	{G,B,G,_},
	{G,B,G,_},
	{G,B,G,_},
	{G,G,G,_},
	{_,_,_,_},
	},

#ifndef ORIG_REPTON
	{ /* 30 */
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{g,0,g,0,0,g,0,g},
	{g,g,g,0,0,g,g,g},
	{0,g,v,g,g,v,g,0},
	{0,g,g,g,g,g,g,0},
	},
#else
	{ /* 30 */
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{G,_,G,_},
	{G,_,G,_},
	{G,G,G,_},
	{G,G,G,_},
	{G,G,G,_},
	},
#endif

#ifndef ORIG_REPTON
	{ /* 31 */
	{0,0,0,0,0,0,0,0},
	{0,0,0,v,v,0,0,0},
	{0,0,0,v,v,0,0,0},
	{0,0,g,g,g,g,0,0},
	{0,g,g,g,g,g,g,0},
	{0,g,0,b,b,0,g,0},
	{0,0,0,b,b,0,0,0},
	{0,0,b,b,b,b,0,0},
	},
#else
	{ /* 31 */
	{_,_,_,_},
	{_,V,V,_},
	{_,V,V,_},
	{G,G,G,G},
	{G,G,G,G},
	{G,G,G,G},
	{_,B,B,_},
	{B,B,B,B},
	},
#endif

	{ /* 32 */
	{_,_,_,_},
	{_,_,_,_},
	{_,_,_,_},
	{_,B,_,_},
	{B,_,B,_},
	{_,B,_,_},
	{_,_,_,_},
	{_,_,_,_},
	},
};

#undef b
#undef g
#undef v
#undef B
#undef G
#undef V
#undef _
