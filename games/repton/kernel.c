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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#include "repton.h"

enum orientation {
	NORTH,
	SOUTH,
	EAST,
	WEST
};

static int try_fall_horizontal(struct map_s *mptr, int x, int y, int xd, int obj);
static void test_fall(struct map_s *mptr, int x, int y, int obj);
static int blip_try_dir(struct map_s *mptr, struct monster_s *ms, int dir, int xd, int yd);
static void test_blip(struct map_s *mptr, struct monster_s *ms);
static double monster_consider_dir(struct map_s *mptr, struct monster_s *ms, int xd, int yd);
static void test_monster(struct map_s *mptr, struct monster_s *ms);
void check_eggs(struct map_s *mptr);
void check_objects(struct map_s *mptr);
void check_plants(struct map_s *mptr);
void check_monsters(struct map_s *mptr);
static void move_vertical_direct(struct map_s *mptr, int yd);
static void move_horizontal_direct(struct map_s *mptr, int xd);
static void gotkey(struct map_s *mptr);
static void got_object(struct map_s *mptr, int obj);
static void go_transport(struct map_s *mptr, int x, int y);
void move_vertical(struct map_s *mptr, int yd);
static void move_horizontal_move_obj(struct map_s *mptr, int xd, int obj);
void move_horizontal(struct map_s *mptr, int xd);
static void remove_monster(struct map_s *mptr, struct monster_s *_ms);
static void kill_monster(struct map_s *mptr, struct monster_s *ms);
static void cage_blip(struct map_s *mptr, struct monster_s *ms, int x, int y);
void add_monster(struct map_s *mptr, int x, int y, int type);
int find_monster(struct map_s *mptr, int x, int y);
struct monster_s *find_monsterptr(struct map_s *mptr, int x, int y);

/*
 * Check if the specified object can fall to the left or to the right.
 */
static int
try_fall_horizontal(struct map_s *mptr, int x, int y, int xd, int obj)
{
	if (((xd == -1 && x > 0) || (xd == 1 && x < MAP_WIDTH - 1)) &&
	    mptr->map[x+xd][y+1] == OBJ_EMPTY &&
	    mptr->map[x+xd][y] == OBJ_EMPTY) {
		mptr->map[x][y] = OBJ_EMPTY;
		if (obj == OBJ_EGG)
			mptr->map[x+xd][y] = OBJ_FALLING_EGG;
		else
			mptr->map[x+xd][y] = obj;
		sound(SND_FALLING_ROCK);
		return 1;
	}
	return 0;
}

/*
 * Check if the specified object can fall.
 */
static void
test_fall(struct map_s *mptr, int x, int y, int obj)
{
	struct monster_s *ms;
	/*
	 * If there is nothing below the object, fall down.
	 */
	if (mptr->map[x][y+1] == OBJ_EMPTY) {
		mptr->map[x][y] = OBJ_EMPTY;
		if ((ms = find_monsterptr(mptr, x, y+1)) != NULL && ms->type == OBJ_MONSTER)
			kill_monster(mptr, ms);
		if (obj == OBJ_EGG)
			mptr->map[x][y+1] = OBJ_FALLING_EGG;
		else
			mptr->map[x][y+1] = obj;
		sound(SND_FALLING_ROCK);
		if (y == MAP_HEIGHT - 2 && (obj == OBJ_EGG || obj == OBJ_FALLING_EGG))
			mptr->map[x][y+1] = OBJ_BROKEN_EGG;
		if (y < MAP_HEIGHT - 2 && mptr->map[x][y+2] == OBJ_REPTON)
			do_die(mptr);
	} else
		switch(mptr->map[x][y+1]) {
		/*
		 * Curved objects.  Rocks and eggs can fall onto
		 * these objects.
		 */
		case OBJ_ROCK:
		case OBJ_SKULL:
		case OBJ_DIAMOND:
		case OBJ_EGG:
		case OBJ_KEY:
		case OBJ_BOMB:
		case OBJ_BROKEN_EGG:
		case OBJ_BROKEN_EGG1:
		case OBJ_BROKEN_EGG2:
		case OBJ_BROKEN_EGG3:
			/*
			 * If can't fall to the left, fall to the right.
			 */
			if (!try_fall_horizontal(mptr, x, y, -1, obj))
				if (!try_fall_horizontal(mptr, x, y, 1, obj) && obj == OBJ_FALLING_EGG)
					mptr->map[x][y] = OBJ_BROKEN_EGG;
			break;
		case OBJ_WALL_NORTH_WEST:
		case OBJ_FILLED_WALL_NORTH_WEST:
			if (!try_fall_horizontal(mptr, x, y, -1, obj) && obj == OBJ_FALLING_EGG)
					mptr->map[x][y] = OBJ_BROKEN_EGG;
			break;		
		case OBJ_WALL_NORTH_EAST:
		case OBJ_FILLED_WALL_NORTH_EAST:
			if (!try_fall_horizontal(mptr, x, y, 1, obj) && obj == OBJ_FALLING_EGG)
					mptr->map[x][y] = OBJ_BROKEN_EGG;
			break;		
		default:
			if (obj == OBJ_FALLING_EGG)
				mptr->map[x][y] = OBJ_BROKEN_EGG;
		}
}

/*
 * Check if the blip can go into a direction.
 */
static int
blip_try_dir(struct map_s *mptr, struct monster_s *ms, int dir, int xd, int yd)
{
	if ((xd == -1 && ms->x > 0) || (yd == -1 && ms->y > 0) ||
	    (xd == 1 && ms->x < MAP_WIDTH - 1) ||
	    (yd == 1 && ms->y < MAP_HEIGHT - 1)) {
		switch (mptr->map[ms->x + xd][ms->y + yd]) {
		case OBJ_EMPTY:
		case OBJ_GROUND1:
		case OBJ_GROUND2:
			ms->x += xd;
			ms->y += yd;
			ms->orientation = dir;
			return 1;
		case OBJ_CAGE:
			cage_blip(mptr, ms, ms->x + xd, ms->y + yd);
			return 1;
		case OBJ_REPTON:
			do_die(mptr);
		}
	}
	return 0;
}

/*
 * Check and move the blip.
 */
static void
test_blip(struct map_s *mptr, struct monster_s *ms)
{
	switch (ms->orientation) {
	case NORTH:
		if (blip_try_dir(mptr, ms, WEST, -1, 0))
			;
		else if (!blip_try_dir(mptr, ms, NORTH, 0, -1)) {
			ms->orientation = EAST;
			test_blip(mptr, ms);
		}
		break;
	case SOUTH:
		if (blip_try_dir(mptr, ms, EAST, 1, 0))
			;
		else if (!blip_try_dir(mptr, ms, SOUTH, 0, 1)) {
			ms->orientation = WEST;
			test_blip(mptr, ms);
		}
		break;
	case EAST:
		if (blip_try_dir(mptr, ms, NORTH, 0, -1))
			;
		else if (!blip_try_dir(mptr, ms, EAST, 1, 0)) {
			ms->orientation = SOUTH;
			test_blip(mptr, ms);
		}
		break;
	case WEST:
		if (blip_try_dir(mptr, ms, SOUTH, 0, 1))
			;
		else if (!blip_try_dir(mptr, ms, WEST, -1, 0)) {
			ms->orientation = NORTH;
			test_blip(mptr, ms);
		}
		break;
	}
	if (ms->move == OBJ_BLIP)
		ms->move = OBJ_BLIP2;
	else
		ms->move = OBJ_BLIP;
}

/*
 * Check if the monster can go into a direction and the distance between
 * it and repton.
 */
static double
monster_consider_dir(struct map_s *mptr, struct monster_s *ms, int xd, int yd)
{
	if ((xd == -1 && ms->x > 0) || (yd == -1 && ms->y > 0) ||
	    (xd == 1 && ms->x < MAP_WIDTH - 1) ||
	    (yd == 1 && ms->y < MAP_HEIGHT - 1)) {
		switch (mptr->map[ms->x + xd][ms->y + yd]) {
		case OBJ_EMPTY:
		case OBJ_GROUND1:
		case OBJ_GROUND2:
			return sqrt(pow(ms->x + xd - mptr->repton_x, 2) +
			            pow(ms->y + yd - mptr->repton_y, 2));
		case OBJ_REPTON:
			do_die(mptr);
		}
	}
	return 99;
}

/*
 * Check the repton position and move the monster according with it.
 */
static void
test_monster(struct map_s *mptr, struct monster_s *ms)
{
	int xd = 0, yd = 0;
	double i, len;

#ifdef RANDOM_MOVES
	if (rand() % 3 != 0) {
#endif
		len = sqrt(pow(ms->x - mptr->repton_x, 2) +
			   pow(ms->y - mptr->repton_y, 2));

		if ((i = monster_consider_dir(mptr, ms, 0, -1)) < len) {
			xd = 0; yd = -1; len = i;
		}
		if ((i = monster_consider_dir(mptr, ms, 0, 1)) < len) {
			xd = 0; yd = 1; len = i;
		}
		if ((i = monster_consider_dir(mptr, ms, 1, 0)) < len) {
			xd = 1; yd = 0; len = i;
		}
		if (monster_consider_dir(mptr, ms, -1, 0) < len) {
			xd = -1; yd = 0;
		}
#ifdef RANDOM_MOVES
	} else {
		int px[4] = {0, 0, 1, -1}, py[4] = {1, -1, 0, 0};
		int m = 0;
		while (m++ < 4) {
			int q = rand() % 4;
			if (px[q] != 2) {
				if (monster_consider_dir(mptr, ms, px[q], py[q]) != 99) {
					xd = px[q];
					yd = py[q];
					break;
				}
				px[q] = 2;
			}
		}
	}
#endif /* RANDOM_MOVES */

	ms->x += xd;
	ms->y += yd;

	if (ms->move == OBJ_MONSTER)
		ms->move = OBJ_MONSTER2;
	else
		ms->move = OBJ_MONSTER;
}

/*
 * Check the broken eggs.
 */
void
check_eggs(struct map_s *mptr)
{
	int x, y;

	for (y = MAP_HEIGHT - 1; y >= 0; y--)
	for (x = MAP_WIDTH - 1; x >= 0; x--)
		switch (mptr->map[x][y]) {
		case OBJ_BROKEN_EGG:
			mptr->map[x][y] = OBJ_BROKEN_EGG1; break;
		case OBJ_BROKEN_EGG1:
			mptr->map[x][y] = OBJ_BROKEN_EGG2; break;
		case OBJ_BROKEN_EGG2:
			mptr->map[x][y] = OBJ_BROKEN_EGG3; break;
		case OBJ_BROKEN_EGG3:
			mptr->map[x][y] = OBJ_EMPTY;
			add_monster(mptr, x, y, OBJ_MONSTER);
			break;
		}
}

/*
 * Check the moveable objects (rocks and eggs).
 */
void
check_objects(struct map_s *mptr)
{
	int x, y;

	/*
	 * Note the ``MAP_HEIGHT - 2'' value.
	 */
	for (y = MAP_HEIGHT - 2; y >= 0; y--)
	for (x = MAP_WIDTH - 1; x >= 0; x--)
		switch(mptr->map[x][y]) {
		case OBJ_ROCK:
		case OBJ_EGG:
		case OBJ_FALLING_EGG:
		case OBJ_BROKEN_EGG:
		case OBJ_BROKEN_EGG1:
		case OBJ_BROKEN_EGG2:
		case OBJ_BROKEN_EGG3:
			/*
			 * Check if the object can fall.
			 */
			test_fall(mptr, x, y, mptr->map[x][y]);
			break;
		}
}

/*
 * Check the plants and reproduce.
 *
 * XXX: might be reimplemented using linked lists.
 */
void
check_plants(struct map_s *mptr)
{
	int x, y, c, p, e;

	if (mptr->plants == 0)
		return;

	/*
	 * Ok, this is ultra dirty code, but seem to work :-)
	 */
	for (y = MAP_HEIGHT - 1; y >= 0; y--)
	for (x = MAP_WIDTH - 1; x >= 0; x--) {
		p = 0;
		if (mptr->map[x][y] == OBJ_PLANT &&
		    rand() % mptr->plants <= mptr->plants / 10 &&
		    ((x > 0 &&              ((c = mptr->map[x-1][y]) == OBJ_EMPTY || (c == OBJ_REPTON && (p = 1))) && (e = 1)) ||
		     (x < MAP_WIDTH - 1 &&  ((c = mptr->map[x+1][y]) == OBJ_EMPTY || (c == OBJ_REPTON && (p = 1))) && (e = 2)) ||
		     (y > 0 &&              ((c = mptr->map[x][y-1]) == OBJ_EMPTY || (c == OBJ_REPTON && (p = 1))) && (e = 3)) ||
		     (y < MAP_HEIGHT - 1 && ((c = mptr->map[x][y+1]) == OBJ_EMPTY || (c == OBJ_REPTON && (p = 1))) && (e = 4)))) {
			if (p)
				do_die(mptr);
			switch (e) {
			case 1: mptr->map[x-1][y] = OBJ_PLANT; break;
			case 2: mptr->map[x+1][y] = OBJ_PLANT; break;
			case 3: mptr->map[x][y-1] = OBJ_PLANT; break;
			case 4: mptr->map[x][y+1] = OBJ_PLANT; break;
			}
			mptr->plants++;
			return;
		}
	}
}

/*
 * Check the monsters status and move them.
 */
void
check_monsters(struct map_s *mptr)
{
	struct monster_s *ms;

	for (ms = mptr->monster_list_head; ms != NULL; ms = ms->next)
		switch (ms->type) {
		case OBJ_BLIP:
			test_blip(mptr, ms);
			break;
		case OBJ_MONSTER:
			test_monster(mptr, ms);
			break;
		}
}

/*
 * Move repton vertically.
 */
static void
move_vertical_direct(struct map_s *mptr, int yd)
{
	/*
	 * Move repton up or down according with the yd value.
	 */
	mptr->map[mptr->repton_x][mptr->repton_y] = OBJ_EMPTY;
	mptr->map[mptr->repton_x][mptr->repton_y + yd] = OBJ_REPTON;
	mptr->repton_y += yd;

	/*
	 * Change the repton sprite orientation.
	 */
	switch (mptr->repton_move) {
	case OBJ_REPTON:
	case OBJ_REPTON_LOOK_LEFT:
	case OBJ_REPTON_LOOK_RIGHT:
	case OBJ_REPTON_GO_RIGHT1:
	case OBJ_REPTON_GO_RIGHT2:
	case OBJ_REPTON_GO_RIGHT3:
	case OBJ_REPTON_GO_RIGHT4:
	case OBJ_REPTON_GO_LEFT1:
	case OBJ_REPTON_GO_LEFT2:
	case OBJ_REPTON_GO_LEFT3:
	case OBJ_REPTON_GO_LEFT4:
	case OBJ_REPTON_GO_UP2:
		mptr->repton_move = OBJ_REPTON_GO_UP1; break;
	case OBJ_REPTON_GO_UP1:
		mptr->repton_move = OBJ_REPTON_GO_UP2; break;
	}
}

/*
 * Move repton horizontally.
 */
static void
move_horizontal_direct(struct map_s *mptr, int xd)
{
	/*
	 * Move repton to the left or to the right according
	 * with the xd value.
	 */
	mptr->map[mptr->repton_x][mptr->repton_y] = OBJ_EMPTY;
	mptr->map[mptr->repton_x + xd][mptr->repton_y] = OBJ_REPTON;
	mptr->repton_x += xd;

	/*
	 * Change the repton sprite orientation.
	 */
	if (xd == -1)
		/*
		 * Move repton to the left.
		 */
		switch (mptr->repton_move) {
		case OBJ_REPTON:
		case OBJ_REPTON_LOOK_LEFT:
		case OBJ_REPTON_LOOK_RIGHT:
		case OBJ_REPTON_GO_UP1:
		case OBJ_REPTON_GO_UP2:
		case OBJ_REPTON_GO_RIGHT1:
		case OBJ_REPTON_GO_RIGHT2:
		case OBJ_REPTON_GO_RIGHT3:
		case OBJ_REPTON_GO_RIGHT4:
		case OBJ_REPTON_GO_LEFT4:
			mptr->repton_move = OBJ_REPTON_GO_LEFT1; break;
		case OBJ_REPTON_GO_LEFT1:
			mptr->repton_move = OBJ_REPTON_GO_LEFT2; break;
		case OBJ_REPTON_GO_LEFT2:
			mptr->repton_move = OBJ_REPTON_GO_LEFT3; break;
		case OBJ_REPTON_GO_LEFT3:
			mptr->repton_move = OBJ_REPTON_GO_LEFT4; break;
		}
	else
		/*
		 * Move repton to the right.
		 */
		switch (mptr->repton_move) {
		case OBJ_REPTON:
		case OBJ_REPTON_LOOK_LEFT:
		case OBJ_REPTON_LOOK_RIGHT:
		case OBJ_REPTON_GO_UP1:
		case OBJ_REPTON_GO_UP2:
		case OBJ_REPTON_GO_LEFT1:
		case OBJ_REPTON_GO_LEFT2:
		case OBJ_REPTON_GO_LEFT3:
		case OBJ_REPTON_GO_LEFT4:
		case OBJ_REPTON_GO_RIGHT4:
			mptr->repton_move = OBJ_REPTON_GO_RIGHT1; break;
		case OBJ_REPTON_GO_RIGHT1:
			mptr->repton_move = OBJ_REPTON_GO_RIGHT2; break;
		case OBJ_REPTON_GO_RIGHT2:
			mptr->repton_move = OBJ_REPTON_GO_RIGHT3; break;
		case OBJ_REPTON_GO_RIGHT3:
			mptr->repton_move = OBJ_REPTON_GO_RIGHT4; break;
		}
}

/*
 * This function is called when a key is get. It makes the strongbox
 * diamonds.
 */
static void
gotkey(struct map_s *mptr)
{
	int x, y;
	if (mptr->havekey == 0) {
		/*
		 * Automagically change all the strongboxes into diamonds.
		 */
		for (y = 0; y < MAP_HEIGHT; y++)
		for (x = 0; x < MAP_WIDTH; x++)
		if (mptr->map[x][y] == OBJ_STRONGBOX)
			mptr->map[x][y] = OBJ_DIAMOND;
		mptr->havekey = 1;
	}
}

/*
 * This function is called when repton get an object.
 */
static void
got_object(struct map_s *mptr, int obj)
{
	switch (obj) {
	case OBJ_DIAMOND:
		sound(SND_GOT_DIAMOND);
		mptr->diamonds--;
		break;
	case OBJ_TIME:
		sound(SND_GOT_TIME);
		break;
	case OBJ_KEY:
		gotkey(mptr);
		sound(SND_GOT_KEY);
		break;
	case OBJ_CROWN:
		sound(SND_GOT_CROWN);
		break;
	case OBJ_BOMB:
		if (mptr->diamonds == 0)
			do_end_level(mptr);
		break;
	}
}

/*
 * Move repton to a new map location when it enters a transport.
 */
static void
go_transport(struct map_s *mptr, int x, int y)
{
	int i;

	/*
	 * Find the transport location.
	 * Every level can have up to 4 transporters.
	 */
	for (i = 0; i < 16; i += 4)
		if (mptr->transporters[i] == x && mptr->transporters[i+1] == y) {
			/*
			 * Remove repton from the original location.
			 */
			mptr->map[x][y] = OBJ_EMPTY;
			mptr->map[mptr->repton_x][mptr->repton_y] = OBJ_EMPTY;

			/*
			 * Move repton to the new location.
			 */
			mptr->repton_x = mptr->transporters[i+2];
			mptr->repton_y = mptr->transporters[i+3];
			mptr->map[mptr->repton_x][mptr->repton_y] = OBJ_REPTON;
			mptr->repton_move = OBJ_REPTON;

			sound(SND_TRANSPORTER);
			/*
			 * Update the graphic screen.
			 */
			do_transport(mptr);
			return;
		}
}

/*
 * Check if repton can move vertically according with the yd value.
 */
void
move_vertical(struct map_s *mptr, int yd)
{
	int obj = mptr->map[mptr->repton_x][mptr->repton_y + yd];

	if ((yd == -1 && mptr->repton_y > 0) || (yd == 1 && mptr->repton_y < MAP_HEIGHT - 1)) {
		if (find_monster(mptr, mptr->repton_x, mptr->repton_y + yd) != 0)
			do_die(mptr);
		switch (obj) {
		case OBJ_DIAMOND:
		case OBJ_TIME:
		case OBJ_CROWN:
		case OBJ_KEY:
			got_object(mptr, obj);
			/* FALLTHROUGH */
		case OBJ_EMPTY:
		case OBJ_GROUND1:
		case OBJ_GROUND2:
			move_vertical_direct(mptr, yd);
			break;
		case OBJ_TRANSPORT:
			go_transport(mptr, mptr->repton_x, mptr->repton_y + yd);
			break;
		case OBJ_SKULL:
		case OBJ_PLANT:
			do_die(mptr);
			break;
		case OBJ_BOMB:
			got_object(mptr, OBJ_BOMB);
		}
	}
}

/*
 * Move repton and a near object horizontally.
 */
static void
move_horizontal_move_obj(struct map_s *mptr, int xd, int obj)
{
	struct monster_s *ms;
	if (((xd == -1 && mptr->repton_x > 1) || (xd == 1 && mptr->repton_x < MAP_WIDTH - 2)) &&
	    mptr->map[mptr->repton_x + xd*2][mptr->repton_y] == OBJ_EMPTY) {
		if ((ms = find_monsterptr(mptr, mptr->repton_x + xd*2, mptr->repton_y)) != NULL && ms->type == OBJ_MONSTER)
			kill_monster(mptr, ms);
		mptr->map[mptr->repton_x + xd*2][mptr->repton_y] = obj;
		move_horizontal_direct(mptr, xd);
	}
}

/*
 * Check if repton can move horizontally according with the xd value.
 */
void
move_horizontal(struct map_s *mptr, int xd)
{
	int obj = mptr->map[mptr->repton_x + xd][mptr->repton_y];

	if ((xd == -1 && mptr->repton_x > 0) || (xd == 1 && mptr->repton_x < MAP_WIDTH - 1)) {
		if (find_monster(mptr, mptr->repton_x + xd, mptr->repton_y) != 0)
			do_die(mptr);
		switch (obj) {
		case OBJ_ROCK:
		case OBJ_EGG:
		case OBJ_BROKEN_EGG:
		case OBJ_BROKEN_EGG1:
		case OBJ_BROKEN_EGG2:
		case OBJ_BROKEN_EGG3:
			move_horizontal_move_obj(mptr, xd, obj);
			break;
		case OBJ_DIAMOND:
		case OBJ_TIME:
		case OBJ_CROWN:
		case OBJ_KEY:
			got_object(mptr, obj);
			/* FALLTHROUGH */
		case OBJ_EMPTY:
		case OBJ_GROUND1:
		case OBJ_GROUND2:
			move_horizontal_direct(mptr, xd);
			break;
		case OBJ_TRANSPORT:
			go_transport(mptr, mptr->repton_x + xd, mptr->repton_y);
			break;
		case OBJ_SKULL:
		case OBJ_PLANT:
			do_die(mptr);
			break;
		case OBJ_BOMB:
			got_object(mptr, OBJ_BOMB);
		}
	}
}

/*
 * Remove a monster from a list.
 */
static void
remove_monster(struct map_s *mptr, struct monster_s *_ms)
{
	struct monster_s *ms;

	for (ms = mptr->monster_list_head; ms != NULL; ms = ms->next)
		if (ms == _ms) {
			if (ms == mptr->monster_list_head) {
				mptr->monster_list_head = mptr->monster_list_head->next;
				if (mptr->monster_list_head != NULL)
					mptr->monster_list_head->prev = NULL;
				if (mptr->monster_list_tail == ms)
					mptr->monster_list_tail = mptr->monster_list_head;
			} else if (ms == mptr->monster_list_tail) {
				mptr->monster_list_tail = mptr->monster_list_tail->prev;
				if (mptr->monster_list_tail != NULL)
					mptr->monster_list_tail->next = NULL;
			} else {
				if (ms->prev != NULL)
					ms->prev->next = ms->next;
				if (ms->next != NULL)
					ms->next->prev = ms->prev;
			}
		}
}

/*
 * Kill a monster and produce a sound.
 */
static void
kill_monster(struct map_s *mptr, struct monster_s *ms)
{
	remove_monster(mptr, ms);
	free(ms);
	sound(SND_KILLED_MONSTER);
	return;
}

/*
 * Kill a blip, make it a diamond, and produce a sound.
 */
static void
cage_blip(struct map_s *mptr, struct monster_s *ms, int x, int y)
{
	mptr->map[x][y] = OBJ_DIAMOND;
	remove_monster(mptr, ms);
	free(ms);
	sound(SND_ENTERED_CAGE);
	return;
}

/*
 * Add a monster to a map.
 */
void
add_monster(struct map_s *mptr, int x, int y, int type)
{
	struct monster_s *ms;

	if ((ms = (struct monster_s *)malloc(sizeof(struct monster_s))) == NULL)
		err(1, NULL);

	ms->next = NULL;
	ms->prev = NULL;
	ms->x = x;
	ms->y = y;
	ms->type = type;
	ms->move = type;
	ms->orientation = WEST;

	if (mptr->monster_list_head == NULL)
		mptr->monster_list_tail = mptr->monster_list_head = ms;
	else {
		mptr->monster_list_tail->next = ms;
		ms->prev = mptr->monster_list_tail;
		mptr->monster_list_tail = ms;
	}
}

/*
 * Find a monster into a map coordinate and return its pointer.
 */
struct monster_s *
find_monsterptr(struct map_s *mptr, int x, int y)
{
	struct monster_s *ms;
	for (ms = mptr->monster_list_head; ms != NULL; ms = ms->next)
		if (ms->x == x && ms->y == y)
			return ms;
	return NULL;
}

/*
 * Find a monster into a map coordinate and return true if found.
 */
int
find_monster(struct map_s *mptr, int x, int y)
{
	struct monster_s *ms = find_monsterptr(mptr, x, y);
	return ms != NULL ? ms->type : 0;
}
