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

#include <SDL.h>

#define MAP_WIDTH	28
#define MAP_HEIGHT	24
#define MAP_SIZE	MAP_WIDTH * MAP_HEIGHT

#define SPRITE_WIDTH	32
#define SPRITE_HEIGHT	32
#define SPRITE_SIZE	SPRITE_WIDTH * SPRITE_HEIGHT

enum object {
	/*
	 * Sprites.
	 */
	OBJ_ROCK = 0,
	OBJ_DIAMOND,
	OBJ_GROUND1,
	OBJ_GROUND2,
	OBJ_TIME,
	OBJ_SKULL,
	OBJ_EMPTY,
	OBJ_WALL,
	OBJ_WALL_WEST,
	OBJ_WALL_EAST,
	OBJ_WALL_NORTH,
	OBJ_WALL_SOUTH,
	OBJ_WALL_NORTH_WEST,
	OBJ_WALL_NORTH_EAST,
	OBJ_WALL_SOUTH_WEST,
	OBJ_WALL_SOUTH_EST,
	OBJ_FILLED_WALL,
	OBJ_FILLED_WALL_NORTH_WEST,
	OBJ_FILLED_WALL_NORTH_EAST,
	OBJ_FILLED_WALL_SOUTH_WEST,
	OBJ_FILLED_WALL_SOUTH_EST,
	OBJ_WALL_MIRROR,
	OBJ_STRONGBOX,
	OBJ_CAGE,
	OBJ_EGG,
	OBJ_KEY,
	OBJ_PLANT,
	OBJ_BOMB,
	OBJ_TRANSPORT,
	OBJ_CROWN,
	OBJ_REPTON,
	OBJ_BLIP,
	OBJ_BLIP2,
	OBJ_MONSTER,
	OBJ_MONSTER2,
	OBJ_REPTON_LOOK_LEFT,
	OBJ_REPTON_LOOK_RIGHT,
	OBJ_REPTON_GO_RIGHT1,
	OBJ_REPTON_GO_RIGHT2,
	OBJ_REPTON_GO_RIGHT3,
	OBJ_REPTON_GO_RIGHT4,
	OBJ_REPTON_GO_LEFT1,
	OBJ_REPTON_GO_LEFT2,
	OBJ_REPTON_GO_LEFT3,
	OBJ_REPTON_GO_LEFT4,
	OBJ_REPTON_GO_UP1,
	OBJ_REPTON_GO_UP2,
        OBJ_BROKEN_EGG,		/* Broken egg at stage 0. */

	/*
	 * Non sprites.
	 */
        OBJ_BROKEN_EGG1,	/* Broken egg at stage 1. */
        OBJ_BROKEN_EGG2,	/* Broken egg at stage 2. */
        OBJ_BROKEN_EGG3,	/* Broken egg at stage 3. */
	OBJ_FALLING_EGG		/* The egg is falling, so will break. */
};

enum sound {
	SND_GOT_DIAMOND,
	SND_GOT_KEY,
	SND_GOT_TIME,
	SND_GOT_CROWN,
	SND_ENTERED_CAGE,
	SND_KILLED_MONSTER,
	SND_DIE,
	SND_TRANSPORTER,
	SND_MOUSE,
	SND_FALLING_ROCK,
	SND_START_LEVEL,
	SND_END_LEVEL,
	SND_NUM_SOUNDS
};

struct monster_s {
	int type;
	int x, y;
	int orientation;
	int move;
	struct monster_s *next;
	struct monster_s *prev;
};

struct map_s {
	unsigned char password[8];
	unsigned char map[MAP_WIDTH][MAP_HEIGHT];
	unsigned char transporters[16];
	unsigned short palette[4];
	unsigned short *sprites[48];
	SDL_Surface *surface[48];
	int timelimit;
	int level;

	int repton_x, repton_y;
	int repton_move;

	int havekey;
	int diamonds;
	int plants;

	struct monster_s *monster_list_head;
	struct monster_s *monster_list_tail;
};

/*
 * kernel.c
 */
void check_objects(struct map_s *mptr);
void check_monsters(struct map_s *mptr);
void check_plants(struct map_s *mptr);
void check_eggs(struct map_s *mptr);
void move_vertical(struct map_s *mptr, int yd);
void move_horizontal(struct map_s *mptr, int xd);
void add_monster(struct map_s *mptr, int x, int y, int type);
int find_monster(struct map_s *mptr, int x, int y);
struct monster_s *find_monsterptr(struct map_s *mptr, int x, int y);

#define go_left(mptr) move_horizontal(mptr, -1)
#define go_right(mptr) move_horizontal(mptr, 1)
#define go_up(mptr) move_vertical(mptr, -1)
#define go_down(mptr) move_vertical(mptr, 1)

/*
 * fileio.c
 */
struct map_s *load_map(char *filename, int level);
void free_map(struct map_s *mptr);
int find_password(char *filename, char *password);

/*
 * showmap.c
 */
void showmap(struct map_s *mptr);

/*
 * svgalib.c
 */
void main_screen(void);
void svgalib_draw_map(struct map_s *mptr);
void svgalib_loop(struct map_s *mptr);
void do_die(struct map_s *mptr);
void do_end_level(struct map_s *mptr);
void do_transport(struct map_s *mptr);

/*
 * sound.c
 */
void (*sound)(int sound);
void sound_speaker(int sound);
void sound_wave(int sound);
void init_wave_device(void);
void done_wave_device(void);
