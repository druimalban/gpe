/*****************************************************************************
 * gsoko/init.c : globals initialization
 *****************************************************************************
 * Copyright (C) 2000 Jean-Michel Grimaldi
 *
 * Author: Jean-Michel Grimaldi <jm@via.ecp.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *****************************************************************************/

#include <stdio.h>	/* FILE, sprintf() */
#include <stdlib.h>     /* getenv */
#include <string.h>     /* strcpy, strcat */
#include <unistd.h>     /* access */

#include "gsoko.h"

gboolean
load_png (const char *name, GdkPixbuf **pixmap)
{
  *pixmap = gdk_pixbuf_new_from_file (name, NULL);

  if (*pixmap)
    return TRUE;

  fprintf (stderr, "couldn't load %s\n", name);

  *pixmap = NULL;
  return FALSE;
}

/* load the pixmaps */
void load_pixmaps(void)
{
#if 0
	/* macro: i = direction index; name = direction name */
	#define load_pxm(a, i, name)	\
		load_png (PREFIX "/share/gsoko/img/" #name ".png", &a##_pxm[i][0]);	\
		load_png (PREFIX "/share/gsoko/img/" #name "1.png", &a##_pxm[i][1]); \
		load_png (PREFIX "/share/gsoko/img/" #name "2.png", &a##_pxm[i][2]); \
		a##_pxm[i][3] = a##_pxm[i][1];	\
		a##_pxm[i][4] = a##_pxm[i][2];
#else
	/* macro: i = direction index; name = direction name */
	#define load_pxm(a, i, name)	\
		load_png (PREFIX "/share/gsoko/img/man.png", &a##_pxm[i][0]);	\
		a##_pxm[i][1] = a##_pxm[i][0];	\
		a##_pxm[i][2] = a##_pxm[i][0];	\
		a##_pxm[i][3] = a##_pxm[i][1];	\
		a##_pxm[i][4] = a##_pxm[i][2];
#endif

	load_pxm(s, 1, left);
	load_pxm(s, 2, right);
	load_pxm(s, 3, up);
	load_pxm(s, 4, down);

	load_pxm(ps, 1, pleft);
	load_pxm(ps, 2, pright);
	load_pxm(ps, 3, pup);
	load_pxm(ps, 4, pdown);
	
	load_png (PREFIX "/share/gsoko/img/wall.png", &wall_pxm);
	load_png (PREFIX "/share/gsoko/img/tile.png", &tile_pxm);
	load_png (PREFIX "/share/gsoko/img/tile2.png", &tile2_pxm);
	load_png (PREFIX "/share/gsoko/img/box.png", &box_pxm);
	load_png (PREFIX "/share/gsoko/img/box2.png", &box2_pxm);
}


/* initialize globals */
void init_var(void)
{
	/* initialize the backing pixmap */
	darea_pxm = gdk_pixmap_new(
		darea->window,
		darea->allocation.width,
		darea->allocation.height,
		-1);

	/* attach the Graphics Context */
	darea_gc = gdk_gc_new (darea_pxm);

	/* initialize level data, character data, and let the show begin! */
	init_level();
	restart();
}

/* init level from $HOME/.gsokorc */
void init_level(void)
{
	char filename[FILENAME_SIZE];
	FILE *file;

	strcpy(filename, getenv("HOME"));
	strcat(filename, "/.gsokorc");
	if (access(filename, R_OK))
	  level = 1;
	else {
	  file = fopen(filename, "r");
	  if (!file) { perror(filename); exit(1); }
	  if (fscanf(file, "%d", &level) != 1)
	    level = 1;
	  fclose(file);
	}
}

/* save level in file HOME/.gsokorc */
void save_level(void)
{
        char filename[FILENAME_SIZE];
	FILE *file;

	strcpy(filename, getenv("HOME"));
	strcat(filename, "/.gsokorc");
	if ((file = fopen(filename, "w")) != NULL) {
	  fprintf(file, "%d\n", level);
	  fclose(file);	
	}
}
