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

#include <SDL.h>

#include "repton.h"
#include "pathnames.h"

#define FADE_NUM 32

#define VGA_MODE G320x200x256

#define WIDTH 320
#define HEIGHT 200

SDL_Surface *scr_physical;

static int dead, endlevel;

void
sdl_init(void)
{
	SDL_Init (SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER);

	scr_physical = SDL_SetVideoMode (320, 200, 16, SDL_SWSURFACE);
	if (scr_physical == NULL) {
		fprintf (stderr, "Unable to set video mode\n");
		exit (1);
	}
}

void
sdl_done(void)
{
	SDL_Quit ();
}

/*
 * This may need some optimizations on slower machines (less than 486)
 */
void
svgalib_draw_map(struct map_s *mptr)
{
	int x, y, mx, my, obj;
	struct monster_s *ms;
	SDL_Rect rect_list[256];
	int nrects = 0;

#ifdef DOUBLE_BUFFERED_GAME
	gl_setcontext(scr_virtual);
#endif

	for (y = 0; y < 6; y++)
	for (x = 0; x < 10; x++) {
		SDL_Rect rect;

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

		rect.x = x * 32;
		rect.y = y * 32;
		rect.w = 32;
		rect.h = 32;

		SDL_BlitSurface (mptr->surface[obj], NULL, scr_physical, &rect);
		rect_list[nrects++] = rect;

		if (ms != NULL && ms->type == OBJ_BLIP)
			SDL_BlitSurface (mptr->surface[ms->move], NULL, scr_physical, &rect);
	}

#ifdef DOUBLE_BUFFERED_GAME
	gl_copyscreen(scr_physical);
	gl_setcontext(scr_physical);
#endif

	SDL_UpdateRects (scr_physical, nrects, rect_list);
}

int keydown[SDLK_LAST];
int key_event;
int wmouse_x, wmouse_y, wmouse_button, wmouse_oldbutton;

#define MOUSE_LEFTBUTTON 1
#define MOUSE_RIGHTBUTTON 2

void
keyboard_update ()
{
	SDL_Event ev;

	key_event = 0;
	wmouse_oldbutton = wmouse_button;

	if (SDL_PollEvent (&ev)) {
		switch (ev.type)
		{
		case SDL_KEYDOWN:
			key_event = 1;
			keydown[ev.key.keysym.sym] = 1;
			break;
		case SDL_KEYUP:
			keydown[ev.key.keysym.sym] = 0;
			break;
		case SDL_MOUSEBUTTONDOWN:
			wmouse_x = ev.button.x;
			wmouse_y = ev.button.y;
			if (ev.button.button == SDL_BUTTON_LEFT)
				wmouse_button |= MOUSE_LEFTBUTTON;
			if (ev.button.button == SDL_BUTTON_RIGHT)
				wmouse_button |= MOUSE_RIGHTBUTTON;
			break;
		case SDL_MOUSEBUTTONUP:
			if (ev.button.button == SDL_BUTTON_LEFT)
				wmouse_button &= ~MOUSE_LEFTBUTTON;
			if (ev.button.button == SDL_BUTTON_RIGHT)
				wmouse_button &= ~MOUSE_RIGHTBUTTON;
			break;
		default:
			break;
		}
	}
  
}

/*
 * Check the keyboard status and do what is need to do.
 */
static int
check_keyboard(struct map_s *mptr)
{
	keyboard_update ();

	if (keydown[SDLK_LEFT]) {
		go_left(mptr);
		svgalib_draw_map(mptr);
	} else if (keydown[SDLK_RIGHT]) {
		go_right(mptr);
		svgalib_draw_map(mptr);
	} else if (keydown[SDLK_UP]) {
		go_up(mptr);
		svgalib_draw_map(mptr);
	} else if (keydown[SDLK_DOWN]) {
		go_down(mptr);
		svgalib_draw_map(mptr);
	}
	else if (keydown[SDLK_m])
		showmap(mptr);
	else if (keydown[SDLK_q] || keydown[SDLK_ESCAPE])
		return 1;

	return 0;
}

#define EGG_COUNT 3
#define MONSTER_COUNT 6
#define PLANT_COUNT 40

void
svgalib_loop(struct map_s *mptr)
{
	int quit = 0;
	int count = 0;

	svgalib_draw_map(mptr);

	dead = endlevel = 0;

	while (!quit && !dead && !endlevel) {
		count++;
		SDL_Delay (40);
		check_objects(mptr);
		quit = check_keyboard(mptr);
		svgalib_draw_map(mptr);
		if ((count % MONSTER_COUNT) == 0)
			check_monsters(mptr);
		if ((count % PLANT_COUNT) == 0)
			check_plants(mptr);
		if ((count % EGG_COUNT) == 0)
			check_eggs(mptr);
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

#if 0
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
#endif
}

#define PALETTE(x)  (pal[x*3] >> 1) | (pal[x*3 + 1] << 5) | ((pal[x*3 + 2] >> 1) << 11)

static void
load_image(char *filename)
{
	char *buf;
	FILE *fin;
	char pal[768];
	int i, j;
	char *d;
	unsigned short *p;

	if ((buf = (char *)malloc(320 * 200)) == NULL)
		err(1, NULL);
	if ((fin = fopen(filename, "r")) == NULL)
		err(1, "%s", NULL);

	fread(pal, 768, 1, fin);
	fread(buf, 64000, 1, fin);
	fclose(fin);

	d = buf;
	p = (unsigned short *)scr_physical->pixels;

	for (j = 0; j < 200; j++)
	{
		for (i = 0; i < 320; i++)
		{
			unsigned char c = *d++;
			*p++ = PALETTE(c);
		}
	}

	free(buf);

	SDL_UpdateRect (scr_physical, 0, 0, 0, 0);
}

#include "font_8x8.c"

void
write_text (int x, int y, char *string)
{
	while (*string) {
		char c = *string++;
		char *d = fontdata_8x8 + (8 * c);
		int i, j;

		for (j = 0; j < 7; j++) {
			unsigned char f = d[j];
			for (i = 0; i < 7; i++) {
				int b = f & (1 << (7 - i));
				unsigned short *v = (unsigned short *)scr_physical->pixels;
				
				if (b)
					v[x + i + ((y + j) * WIDTH)] = 0xffff;
			}
		}

		x += 8;
	}

	SDL_UpdateRect (scr_physical, 0, 0, 0, 0);
}

void
clearscreen (void)
{
	memset (scr_physical->pixels, 0, WIDTH * HEIGHT * 2);

	SDL_UpdateRect (scr_physical, 0, 0, 0, 0);
}

#define WRITE_CENTERED(y,buf) \
	write_text(WIDTH/2 - strlen(buf) * 8 / 2, y, buf)

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
	load_image(PATH_BITMAPS "/main.bmp");
	sprintf(buf, "Episode %s - Level %d", episode_name(episode), level+1);
//	gl_setwritemode(WRITEMODE_MASKED);
	WRITE_CENTERED(190, buf);
}

void
wait_until_event(void)
{
	for (;;) {
		keyboard_update ();
		if (key_event)
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

	clearscreen();
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
		keyboard_update();

		if (keydown[SDLK_q] || keydown[SDLK_ESCAPE])
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

	load_image(PATH_BITMAPS "/passwd.bmp");

	WRITE_CENTERED(35, episode_name(file));
	WRITE_CENTERED(50, "Enter Password");

#if 0
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

#endif

	redraw_main_screen(file, *level);
	usleep(250000);
}

static void
show_credits(char *file, int level)
{

	load_image(PATH_BITMAPS "/credits.bmp");

	wait_until_event();

	redraw_main_screen(file, level);
	usleep(250000);
}

static void
draw_end_level(void)
{
	int i, j;

#if 0
	for (i = 0; i < WIDTH / FADE_NUM; i++) {
		for (j = 0; j < FADE_NUM; j++)
		gl_fillbox(i+(320/FADE_NUM)*j, 0, 1, HEIGHT, 0);
		usleep(30000);
	}
#endif

	load_image(PATH_BITMAPS "/endlevel.bmp");

	wait_until_event();	
}

static void
draw_end_episode(void)
{
	int i, j;

#if 0
	for (i = 0; i < WIDTH / FADE_NUM; i++) {
		for (j = 0; j < FADE_NUM; j++)
		gl_fillbox(i+(320/FADE_NUM)*j, 0, 1, HEIGHT, 0);
		usleep(30000);
	}
#endif

	load_image(PATH_BITMAPS "/endepis.bmp");

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

	load_image(PATH_BITMAPS "/start.bmp");

	sprintf(buf, "Episode %s", episode_name(file));
	WRITE_CENTERED(45, buf);
	sprintf(buf, "Level %d", *level+1);
	WRITE_CENTERED(70, buf);
	sprintf(buf, "Password %s", current_map->password);
	WRITE_CENTERED(95, buf);

	sound(SND_START_LEVEL);
	wait_until_event();

	clearscreen();

	svgalib_loop(current_map);

	//	gl_setfont(8, 8, font2);
	free_map(current_map);

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

	sdl_init();

	redraw_main_screen(episode, level);

	for (;;) {
		keyboard_update ();

		if (keydown[SDLK_q] || keydown[SDLK_ESCAPE])
			break;

		if (keydown[SDLK_s])
			start_level(episode, &level);
		if (keydown[SDLK_e])
			select_episode(episode, &level);
		if (keydown[SDLK_p])
			select_password(episode, &level);
		if (keydown[SDLK_c])
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
	sdl_done();
}
