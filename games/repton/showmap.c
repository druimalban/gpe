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

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "repton.h"

#include "mapsprites.c"

static void
rectangle(int x0, int y0, int x1, int y1, int c)
{
#if 0
	line(x0, y0, x1, y0, c);
	line(x0, y0, x0, y1, c);
	line(x0, y1, x1, y1, c);
	line(x1, y0, x1, y1, c);
#endif
}

extern SDL_Surface *scr_physical;

void
gl_putbox(struct map_s *mptr, int x, int y, int w, int h, mapobj o)
{
	int i, j;
	unsigned char *p = (unsigned char *)o;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			unsigned char d = p[i + (8 * j)];
			unsigned short *v = (unsigned short *)scr_physical->pixels + x + i + ((j + y) * 320);
			*v = mptr->palette[d];
		}
	}
}

void
showmap(struct map_s *mptr)
{
	int x, y;
	char buf[128];

	clearscreen();

	for (y = 0; y < 24; y++)
	for (x = 0; x < 28; x++) {
		if (mptr->map[x][y] < 32)
			gl_putbox(mptr, 1 + x * 8, 1 + y * 8, 8, 8, mapobjs[mptr->map[x][y]]);
		switch (find_monster(mptr, x, y)) {
		case OBJ_BLIP:
			gl_putbox(mptr, 1 + x * 8, 1 + y * 8, 8, 8, mapobjs[OBJ_BLIP]);
			break;
		case OBJ_MONSTER:
			/* gl_putboxmask(1 + x * 8, 1 + y * 8, 8, 8, mapobjs[OBJ_MONSTER]); */
			break;
		}
	}
	SDL_UpdateRect (scr_physical, 0, 0, 0, 0);

	rectangle(0, 0, 28 * 8, 24 * 8, 15);

	write_text(252, 10, "Level");
	sprintf(buf, "%d", mptr->level);
	write_text(268, 20, buf);
	write_text(240, 40, "Diamonds");
	sprintf(buf, "%d", mptr->diamonds);
	write_text(272-(strlen(buf)*8/2), 50, buf);
	
	usleep(100);

	wait_until_event ();

	clearscreen();
}
