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

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include <vga.h>
#include <vgagl.h>
#include <vgakeyboard.h>
#include <vgamouse.h>

#include "repton.h"
#include "mouse.h"
#include "pathnames.h"

#include "palette.c"

#define FADE_NUM 32

#define VGA_MODE G320x200x256

GraphicsContext *scr_virtual, *scr_physical;

static int dead, endlevel;
static void *font1, *font2;

void
svgalib_init(void)
{
	/*
	 * Initialize the VGA library.
	 * Allocate the buffer for double buffering.
	 */
	if (!vga_hasmode(VGA_MODE))
		errx(1, "VGA mode %d not available", VGA_MODE);

	gl_setcontextvgavirtual(VGA_MODE);

	scr_virtual = gl_allocatecontext();
	gl_getcontext(scr_virtual);

	vga_init();
	vga_setmousesupport(1);
	vga_setmode(VGA_MODE);
	gl_setcontextvga(VGA_MODE);

	scr_physical = gl_allocatecontext();
	gl_getcontext(scr_physical);

	/*
	 * Load the default 8x8 fixed font.
	 */
	if ((font1 = malloc(256 * 8 * 8 * BYTESPERPIXEL)) == NULL)
		err(1, NULL);
	gl_expandfont(8, 8, 15, gl_font8x8, font1);
	if ((font2 = malloc(256 * 8 * 8 * BYTESPERPIXEL)) == NULL)
		err(1, NULL);
	gl_expandfont(8, 8, 255, gl_font8x8, font2);
	gl_setfont(8, 8, font2);

	gl_setcontext(scr_physical);

	gl_setrgbpalette();

	/*
	 * Initialize the keyboard handler.
	 * This seem to hang sometimes the PC.
	 * Hint: don't press too much keys together.
	 */
	if (keyboard_init())
		errx(1, "cannot initialize keyboard");

	keyboard_translatekeys(TRANSLATE_CURSORKEYS | TRANSLATE_KEYPADENTER |
			       TRANSLATE_DIAGONAL);
}

void
svgalib_done(void)
{
	/*
	 * Free the double buffering buffers.
	 */
	gl_freecontext(scr_virtual);
	gl_freecontext(scr_physical);

	/*
	 * Return keyboard to normal state.
	 */
	keyboard_close();

	/*
	 * Return to text mode.
	 */
	vga_setmode(TEXT);
}

/*
 * This may need some optimizations on slower machines (less than 486)
 */
void
svgalib_draw_map(struct map_s *mptr)
{
	int x, y, mx, my, obj;
	struct monster_s *ms;

#ifdef DOUBLE_BUFFERED_GAME
	gl_setcontext(scr_virtual);
#endif

	for (y = 0; y < 6; y++)
	for (x = 0; x < 10; x++) {
		mx = x + mptr->repton_x - 4;
		my = y + mptr->repton_y - 3;

		if (mx < 0 || mx >= MAP_WIDTH || my < 0 || my >= MAP_HEIGHT)
			obj = OBJ_WALL;
		else
			switch (obj = mptr->map[mx][my]) {
			case OBJ_BROKEN_EGG1:
			case OBJ_BROKEN_EGG2:
			case OBJ_BROKEN_EGG3:
				obj = OBJ_BROKEN_EGG; break;
			case OBJ_FALLING_EGG:
				obj = OBJ_EGG; break;
			case OBJ_REPTON:
				obj = mptr->repton_move;
			}

		if ((ms = find_monsterptr(mptr, mx, my)) != NULL && ms->type == OBJ_MONSTER)
				obj = ms->move;

		gl_putbox(x * 32, y * 32, 32, 32, mptr->sprites[obj]);

		if (ms != NULL && ms->type == OBJ_BLIP)
			gl_putboxmask(x * 32, y * 32, 32, 32, mptr->sprites[ms->move]);
	}

#ifdef DOUBLE_BUFFERED_GAME
	gl_copyscreen(scr_physical);
	gl_setcontext(scr_physical);
#endif
}

/*
 * Check the keyboard status and do what is need to do.
 */
static int
check_keyboard(struct map_s *mptr)
{
	keyboard_update();
	if (keyboard_keypressed(SCANCODE_CURSORLEFT)) {
		go_left(mptr);
		svgalib_draw_map(mptr);
	} else if (keyboard_keypressed(SCANCODE_CURSORRIGHT)) {
		go_right(mptr);
		svgalib_draw_map(mptr);
	} else if (keyboard_keypressed(SCANCODE_CURSORUP)) {
		go_up(mptr);
		svgalib_draw_map(mptr);
	} else if (keyboard_keypressed(SCANCODE_CURSORDOWN)) {
		go_down(mptr);
		svgalib_draw_map(mptr);
	} else if (keyboard_keypressed(SCANCODE_M))
		showmap(mptr);
	else if (keyboard_keypressed(SCANCODE_Q) || keyboard_keypressed(SCANCODE_ESCAPE))
		return 1;
	return 0;
}

/*
 * The following delay values may require some tuning.
 * The default values works fine on a 486dx2/66.
 */
#define REDRAW_DELAY		10
#define OBJECT_CHECK_DELAY	5
#define EGG_CHECK_DELAY		55
#define MONSTER_CHECK_DELAY	20
#define PLANT_CHECK_DELAY	500
#define KEYBOARD_CHECK_DELAY	10

void
svgalib_loop(struct map_s *mptr)
{
	int cl, last_redraw, last_kbcheck, last_objcheck, last_eggcheck, last_moncheck, last_plcheck;
	int quit = 0;

	gl_setpalette(&game_palette);

	svgalib_draw_map(mptr);

	last_redraw = last_kbcheck = last_objcheck = last_eggcheck = last_moncheck = last_plcheck = clock();

	dead = endlevel = 0;

	while (!quit && !dead && !endlevel) {
		cl = clock();
		if (cl - last_kbcheck >= KEYBOARD_CHECK_DELAY) {
			quit = check_keyboard(mptr);
			last_kbcheck = clock();
		}
		if (cl - last_redraw >= REDRAW_DELAY) {
			svgalib_draw_map(mptr);
			last_redraw = clock();
		} else {
			if (cl - last_objcheck >= OBJECT_CHECK_DELAY) {
				check_objects(mptr);
				last_objcheck = clock();
			}
			if (cl - last_moncheck >= MONSTER_CHECK_DELAY) {
				check_monsters(mptr);
				last_moncheck = clock();
			}
			if (cl - last_plcheck >= PLANT_CHECK_DELAY) {
				check_plants(mptr);
				last_plcheck = clock();
			}
			if (cl - last_eggcheck >= EGG_CHECK_DELAY) {
				check_eggs(mptr);
				check_objects(mptr);
				last_objcheck = last_eggcheck = clock();
			}
		}
	}
}

void
do_die(struct map_s *mptr)
{
	dead = 1;
	sound(SND_DIE);
}

void
do_end_level(struct map_s *mptr)
{
	endlevel = 1;
	sound(SND_END_LEVEL);
}

void
do_transport(struct map_s *mptr)
{
	int i, j;

	gl_setcontext(scr_virtual);
	gl_clearscreen(0);
	svgalib_draw_map(mptr);
	gl_setcontext(scr_physical);

	for (i = 0; i < WIDTH / FADE_NUM; i++) {
		for (j = 0; j < FADE_NUM; j++)
		gl_copyboxfromcontext(scr_virtual, i+(320/FADE_NUM)*j, 0, 1, HEIGHT,
						   i+(320/FADE_NUM)*j, 0);
		usleep(60000);
	}
}

static void
load_image(char *filename)
{
	char *buf;
	FILE *fin;

	if ((buf = (char *)malloc(320 * 200)) == NULL)
		err(1, NULL);
	if ((fin = fopen(filename, "r")) == NULL)
		err(1, "%s", NULL);
	fread(buf, 768, 1, fin);
	gl_setpalette(buf);
	fread(buf, 64000, 1, fin);
	gl_putbox(0, 0, 320, 200, buf);
	fclose(fin);	
	free(buf);
}

#define WRITE_CENTERED(y,buf) \
	gl_write(WIDTH/2 - strlen(buf) * 8 / 2, y, buf)

static char *
episode_name(char *episode)
{
	static char buf[128];
	char *p;
	if ((p = strrchr(episode, '/')) == NULL)
		p = episode;
	else
		p++;
	strcpy(buf, p);
	*(strrchr(buf, '.')) = '\0';
	return buf;
}

static void
redraw_main_screen(char *episode, int level)
{
	char buf[128];
	wmouse_hide();
	load_image(PATH_BITMAPS "/main.bmp");
	sprintf(buf, "Episode %s - Level %d", episode_name(episode), level+1);
	gl_setwritemode(WRITEMODE_MASKED);
	WRITE_CENTERED(190, buf);
	wmouse_show();
}

static void
wait_until_event(void)
{
	for (;;) {
		wmouse_update();
		keyboard_update();

		if (keyboard_keypressed(SCANCODE_Q) || keyboard_keypressed(SCANCODE_ESCAPE) ||
		    keyboard_keypressed(SCANCODE_ENTER) || keyboard_keypressed(SCANCODE_SPACE))
			break;
		if (wmouse_button & MOUSE_LEFTBUTTON && !wmouse_oldbutton) 
			break;
	}
}

struct episode_s {
	char *filename;
	int selected;
	struct episode_s *next;
};

static struct episode_s *
make_episode_list(char *d)
{
	DIR *dir;
	struct dirent *de;
	struct episode_s *ep, *head = NULL, *last;
	int count = 0;

	if ((dir = opendir(d)) == NULL)
		err(1, "%s", d);
	while ((de = readdir(dir)) != NULL) {
		if (strlen(de->d_name) > 4 && !strcmp(de->d_name + strlen(de->d_name) - 4, ".rep")) {
			if ((ep = (struct episode_s *)malloc(sizeof(struct episode_s))) == NULL)
				err(1, NULL);
			if ((ep->filename = (char *)malloc(strlen(de->d_name) + 1)) == NULL)
				err(1, NULL);
			strcpy(ep->filename, de->d_name);
			ep->selected = 0;
			ep->next = NULL;
			count++;
			if (head == NULL)
				head = ep;
			else
				last->next = ep;
			last = ep;
		}
	}
	closedir(dir);

	if (count == 0)
		errx(1, "%s: no episode map found", d);

	return head;
}

static int
display_episode_list(struct episode_s *ep)
{
	int i;
	char buf[128];

	wmouse_hide();

	gl_clearscreen(0);
	gl_setwritemode(WRITEMODE_OVERWRITE);
	WRITE_CENTERED(180, "Select episode");

	for (i = 0; ep != NULL; ep = ep->next, i++) {
		if (ep->selected) {
			strcpy(buf, "-> ");
			strcat(buf, episode_name(ep->filename));
			strcat(buf, " <-");
		} else
			strcpy(buf, episode_name(ep->filename));
		WRITE_CENTERED(i*8, buf);
	}
	wmouse_show();
	return i;
}

static void
select_episode(char *file, int *level)
{
	struct episode_s *ep, *ep1, *next;
	int i, n;

	ep = make_episode_list(PATH_MAPS);
	for (ep1 = ep; ep1 != NULL; ep1 = ep1->next)
		if (!strcmp(ep1->filename, file))
			ep1->selected = 1;

	n = display_episode_list(ep);

	for (;;) {
		wmouse_update();
		keyboard_update();

		if (keyboard_keypressed(SCANCODE_Q) || keyboard_keypressed(SCANCODE_ESCAPE))
			break;
		if (wmouse_button & MOUSE_RIGHTBUTTON && !wmouse_oldbutton)
			break;
		if (wmouse_button & MOUSE_LEFTBUTTON && !wmouse_oldbutton) {
			if (wmouse_y > n * 8)
				sound(SND_MOUSE);
			else {
				for (ep1 = ep; ep1 != NULL; ep1 = ep1->next)
					ep1->selected = 0;
				for (ep1 = ep, i = 0; ep1 != NULL; ep1 = ep1->next, i++)
					if (wmouse_y >= i*8 && wmouse_y <= i*8 + 8) {
						ep1->selected = 1;
						strcpy(file, ep1->filename);
						*level = 0;
						break;
					}				
				display_episode_list(ep);
			}
		}
	}

	redraw_main_screen(file, *level);
	usleep(250000);

	while (ep != NULL) {
		next = ep->next;
		free(ep->filename);
		free(ep);
		ep = next;
	}
}

static void
select_password(char *file, int *level)
{
	char buf[128], pwd[9] = "";
	int c, n = 0, i;

	wmouse_hide();

	load_image(PATH_BITMAPS "/passwd.bmp");

	gl_setwritemode(WRITEMODE_OVERWRITE);
	WRITE_CENTERED(35, episode_name(file));
	WRITE_CENTERED(50, "Enter Password");

	keyboard_close();

	for (;;) {
		gl_fillbox(100, 65, 100, 10, 0);
		sprintf(buf, "[%s]", pwd);
		WRITE_CENTERED(65, buf);

		c = vga_getch();

		if (c == '\n') {
			strcpy(buf, PATH_MAPS);
			strcat(buf, "/");
			strcat(buf, file);
			if ((i = find_password(buf, pwd)) != -1) {
				*level = i;
				sprintf(buf, "Level %d", i+1);
				WRITE_CENTERED(80, "Password Accepted");
				WRITE_CENTERED(90, buf);
				WRITE_CENTERED(100, "Press a key");
				vga_getch();
				break;
			} else {
				WRITE_CENTERED(85, "Wrong Password");
				WRITE_CENTERED(95, "Press a key");
				vga_getch();
				break;
			}
		} else if (c == 27)
			break;
		else if (c == 127) {
			if (n > 0)
				pwd[--n] = '\0';
			else
				sound(SND_MOUSE);
		} else if (isalnum(c)) {
			if (n < 7) {
				pwd[n] = toupper(c);
				pwd[++n] = '\0';
			} else
				sound(SND_MOUSE);
		} else
			sound(SND_MOUSE);
	}

	wmouse_show();
	keyboard_init();
	redraw_main_screen(file, *level);
	usleep(250000);
}

static void
show_credits(char *file, int level)
{
	wmouse_hide();

	load_image(PATH_BITMAPS "/credits.bmp");

	wmouse_show();

	wait_until_event();

	redraw_main_screen(file, level);
	usleep(250000);
}

static void
draw_end_level(void)
{
	int i, j;

	wmouse_hide();

	for (i = 0; i < WIDTH / FADE_NUM; i++) {
		for (j = 0; j < FADE_NUM; j++)
		gl_fillbox(i+(320/FADE_NUM)*j, 0, 1, HEIGHT, 0);
		usleep(30000);
	}

	load_image(PATH_BITMAPS "/endlevel.bmp");

	wmouse_show();

	wait_until_event();	
}

static void
draw_end_episode(void)
{
	int i, j;

	wmouse_hide();

	for (i = 0; i < WIDTH / FADE_NUM; i++) {
		for (j = 0; j < FADE_NUM; j++)
		gl_fillbox(i+(320/FADE_NUM)*j, 0, 1, HEIGHT, 0);
		usleep(30000);
	}

	load_image(PATH_BITMAPS "/endepis.bmp");

	wmouse_show();

	wait_until_event();	
}

static void
start_level(char *file, int *level)
{
	char buf[128];
	struct map_s *current_map;

	srand(time(NULL));
	strcpy(buf, PATH_MAPS);
	strcat(buf, "/");
	strcat(buf, file);
	current_map = load_map(buf, *level);

	wmouse_hide();
	load_image(PATH_BITMAPS "/start.bmp");
	sprintf(buf, "Episode %s", episode_name(file));
	WRITE_CENTERED(45, buf);
	sprintf(buf, "Level %d", *level+1);
	WRITE_CENTERED(70, buf);
	sprintf(buf, "Password %s", current_map->password);
	WRITE_CENTERED(95, buf);

	wmouse_show();
	sound(SND_START_LEVEL);
	wait_until_event();
	wmouse_hide();

	gl_clearscreen(0);

	gl_setfont(8, 8, font1);
	svgalib_loop(current_map);
	gl_setfont(8, 8, font2);
	free_map(current_map);

	wmouse_show();

	if (endlevel) {
		if (*level < 7) {
			draw_end_level();
			(*level)++;
		} else {
			draw_end_episode();
			*level = 0;
		}
	}
	
	redraw_main_screen(file, *level);
	usleep(250000);
}

#define START_BUTTON()	  (wmouse_x >= 69 && wmouse_y >= 72 && wmouse_x <= 259 && wmouse_y <= 98)
#define PASSWORD_BUTTON() (wmouse_x >= 69 && wmouse_y >= 102 && wmouse_x <= 259 && wmouse_y <= 128)
#define EPISODE_BUTTON()  (wmouse_x >= 69 && wmouse_y >= 132 && wmouse_x <= 259 && wmouse_y <= 158)
#define QUIT_BUTTON()     (wmouse_x >= 69 && wmouse_y >= 162 && wmouse_x <= 259 && wmouse_y <= 188)
#define CREDITS_BUTTON()  (wmouse_x >= 85 && wmouse_y >= 14 && wmouse_x <= 240 && wmouse_y <= 54)

void
main_screen(void)
{
	int level = 0;
	char episode[128] = "prelude.rep";

	wmouse_color_white = 255;
	wmouse_color_black = 0;

	svgalib_init();

	wmouse_reset();
	wmouse_show();

	redraw_main_screen(episode, level);

	for (;;) {
		wmouse_update();
		keyboard_update();

		if (keyboard_keypressed(SCANCODE_Q) || keyboard_keypressed(SCANCODE_ESCAPE))
			break;

		if (keyboard_keypressed(SCANCODE_S))
			start_level(episode, &level);
		if (keyboard_keypressed(SCANCODE_E))
			select_episode(episode, &level);
		if (keyboard_keypressed(SCANCODE_P))
			select_password(episode, &level);
		if (keyboard_keypressed(SCANCODE_C))
			show_credits(episode, level);

		if (wmouse_button & MOUSE_LEFTBUTTON && !wmouse_oldbutton) {
			if (START_BUTTON())
				start_level(episode, &level);
			else if (CREDITS_BUTTON())
				show_credits(episode, level);
			else if (PASSWORD_BUTTON())
				select_password(episode, &level);
			else if (EPISODE_BUTTON())
				select_episode(episode, &level);
			else if (QUIT_BUTTON())
				break;
			else
				sound(SND_MOUSE);
		}
	}
	svgalib_done();
}
