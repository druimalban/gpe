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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "repton.h"

#include "palette.c"

#undef SHOW_PASSWORDS

#define PASSWORD_OFFSET		0
#define TIME_LIMIT_OFFSET	64
#define EDITOR_CODES_OFFSET	80
#define TRANSPORTER_OFFSET	96
#define PALETTE_OFFSET		224
#define MAP_OFFSET		256
#define SPRITE_OFFSET		3616

static void
decode_pwd(char *buf)
{
	int i;
	for (i = 63; i >= 0; i--) {
		int c = *buf ^= i;
		if (c == 0x0d) c = '\0';
		*buf++ = c;
	}
}

static void
translate_map(struct map_s *mptr, char *buf)
{
	int c, x, y;
	int found = 0;

	for (y = 0; y < MAP_HEIGHT; y++)
	for (x = 0; x < MAP_WIDTH; x++)
		switch (c = buf[x + y * MAP_WIDTH]) {
		case OBJ_BLIP:
			mptr->map[x][y] = OBJ_EMPTY;
			add_monster(mptr, x, y, OBJ_BLIP);
			break;
		case OBJ_STRONGBOX:
		case OBJ_CAGE:
		case OBJ_DIAMOND:
			mptr->map[x][y] = c;
			mptr->diamonds++;
			break;
		case OBJ_PLANT:
			mptr->map[x][y] = OBJ_PLANT;
			mptr->plants++;
			break;
		case OBJ_REPTON:
			mptr->repton_x = x;
			mptr->repton_y = y;
			found++;
			/* FALLTHROUGH */
		default:
			mptr->map[x][y] = c;
		}

	if (found != 1)
		errx(1, "wrong number of repton: %d", found);
}

static void
revbits(int *c, int n)
{
	int i, c1 = 0;
	for (i = 0; i < n; i++) {
		c1 = (c1 << 1) | (*c & 1);
		*c >>= 1;
	}
	*c = c1;
}

static void
decode_map(struct map_s *mptr, char *in)
{
	unsigned char buf[MAP_SIZE];
	char *p = in;
	int i, j, inbyte, outbyte, bits = 0;

	for (i = 0; i < MAP_SIZE; i++) {
		outbyte = 0;
		for (j = 0; j < 5; j++) {
			if (--bits <= 0) {
				inbyte = *p++;
				bits = 8;
			}
			outbyte = (outbyte << 1) | (inbyte & 1);
			inbyte >>= 1;
		}
		revbits(&outbyte, 5);
		buf[i] = outbyte;
	}

	translate_map(mptr, buf);
}

static void
decode_palette(struct map_s *mptr, char *in)
{
	static int trans[] = {0, 12, 10, 14, 9, 13, 11, 15};
	int i;
	for (i = 0; i < 4; i++) {
	  int c = trans[(int)in[i]];
	  mptr->palette[i] = game_palette[c].r | (game_palette[c].g << 5) | (game_palette[c].b << 11);
	}
}

static int exbits(unsigned char c, unsigned int bit)
{
  return ((c & (1 << bit)) >> bit)
    | ((c & (1 << (4 + bit))) >> (3 + bit));
}

/*
 * This function requires a rewrite.
 */
static void
decode_sprite(struct map_s *mptr, char *in, int n)
{
	int i, j, x, y;

	if ((mptr->sprites[n] = (char *)malloc(SPRITE_WIDTH * SPRITE_HEIGHT * 2)) == NULL)
		err(1, NULL);

	x = y = 0;
	for (i = 0; i < 128; i++) {
		for (j = 0; j < 4; j++) {
			static char v[4] = {8, 4, 2, 1};
			*(mptr->sprites[n] + (x+j)*2 + y * SPRITE_WIDTH) =
			*(mptr->sprites[n] + (x+j)*2 + y * SPRITE_WIDTH + 1) =
			mptr->palette[((in[i] & v[j]) / v[j]) + ((in[i] & (v[j] << 4)) / (v[j] << 3))];
		}
		y++;
		if (y % 8 == 0) {
			x += 4;
			y = ((y / 8 - 1) * 8);
			if (x == 16) {
				x = 0;
				y += 8;
			}
		}
	}

	mptr->surface[n] = SDL_CreateRGBSurfaceFrom (mptr->sprites[n], SPRITE_WIDTH, SPRITE_HEIGHT,
						     16, SPRITE_WIDTH * 2, 0x1f, 0x3f << 5, 0x1f << 11, 0);
}

void
free_map(struct map_s *mptr)
{
	struct monster_s *ms, *next;
	int i;

	for (ms = mptr->monster_list_head; ms != NULL;) {
		next = ms->next;
		free(ms);
		ms = next;
	}
	for (i = 0; i < 48; i++)
		free(mptr->sprites[i]);

	free(mptr);
}

static void
xfread(void *buf, size_t size, size_t n, FILE *stream, char *filename)
{
	if (fread(buf, size, n, stream) != n)
		errx(1, "%s:unexpected end of file", filename);
}

int
find_password(char *filename, char *pwd)
{
	FILE *fin;
	int i;
	char buf[8][8];

	if ((fin = fopen(filename, "r")) == NULL)
		err(1, "%s", filename);

	fread(buf, sizeof(buf), 1, fin);

	fclose(fin);

	decode_pwd(buf[0]);
	for (i = 0; i < 8; i++) {
#ifdef SHOW_PASSWORDS
	printf("level %d: password \"%s\"\n", i, buf[i]);
#endif
		if (!strcmp(pwd, buf[i]))
			return i;
	}

	return -1;
}

struct map_s *
load_map(char *filename, int level)
{
	FILE *fin;
	struct map_s *mptr;
	int i;
	char buf[1024];
	char pwd[8][8];

	assert(level >= 0 && level <= 7);

	if ((mptr = (struct map_s *)malloc(sizeof(struct map_s))) == NULL)
		err(1, NULL);

	mptr->monster_list_head = mptr->monster_list_tail = NULL;
	mptr->havekey = 0;
	mptr->diamonds = 0;
	mptr->plants = 0;
	mptr->level = level + 1;
	mptr->repton_move = OBJ_REPTON;

	if ((fin = fopen(filename, "r")) == NULL)
		err(1, "%s", filename);

	/*
	 * Load and decode the password.
	 */
	fread(pwd, sizeof(pwd), 1, fin); 
	decode_pwd(pwd[0]);
	strcpy(mptr->password, pwd[level]);

	/*
	 * Load the time limit.
	 */
	fseek(fin, TIME_LIMIT_OFFSET + level * 2, SEEK_SET);
	xfread(&mptr->timelimit, 2, 1, fin, filename);

	/*
	 * Load the transporter destinations.
	 */
	fseek(fin, TRANSPORTER_OFFSET + level * 16, SEEK_SET);
	xfread(mptr->transporters, 16, 1, fin, filename);

	/*
	 * Load the color palette.
	 */
	fseek(fin, PALETTE_OFFSET + level * 4, SEEK_SET);
	xfread(buf, 4, 1, fin, filename);
	decode_palette(mptr, buf);

	/*
	 * Load the game map.
	 */
	fseek(fin, MAP_OFFSET + level * (MAP_WIDTH*MAP_HEIGHT*5/8), SEEK_SET);
	xfread(buf, (MAP_WIDTH*MAP_HEIGHT*5/8), 1, fin, filename);
	decode_map(mptr, buf);

	/*
	 * Load the sprites.
	 */
	fseek(fin, SPRITE_OFFSET, SEEK_SET);
	for (i = 0; i < 48; i++) {
		fread(buf, 16 * 32 * 2 / 8, 1, fin);
		decode_sprite(mptr, buf, i);
	}

	fclose(fin);

	return mptr;
}
