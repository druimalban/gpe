/*****************************************************************************
 * gsoko/gsoko.h : globals declaration
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

#ifndef __GSOKO_H__
#define __GSOKO_H__

#include <gtk/gtk.h>

/*------------------------------------*/
/* Constants                          */
/*------------------------------------*/

/* window title */
#define W_TITLE	"gSoko"

/* number of levels */
#define N_LEVELS	50

/* dimensions of the board (unit = tile) */
#define W_BOARD	20
#define H_BOARD	16

/* dimensions of the background tiles */
#define W_TILE	12
#define H_TILE	12

/* dimensions of the drawing area (unit = pixel) */
#define W_DAREA	W_TILE * W_BOARD
#define H_DAREA	H_TILE * H_BOARD

/* dimensions of the sprite */
#define W_S	W_TILE
#define H_S	H_TILE

/* delay between 2 steps in the animation (in msecs) */
#define ANIM_DELAY	5

/* key codes */
#define K_LEFT	0xff51
#define K_RIGHT	0xff53
#define K_UP	0xff52
#define K_DOWN	0xff54
#define K_J	0x6a
#define K_RETURN 0xff0d

/* other */
#define FILENAME_SIZE 128
#define HIST_SIZE     4096

/*------------------------------------*/
/* Structure                          */
/*------------------------------------*/

typedef struct {
  int dir, push;
} History;

/*------------------------------------*/
/* Global variables                   */
/*------------------------------------*/

GtkWidget *window;	/* main window */
GtkWidget *box;		/* container */
GtkWidget *darea;	/* drawing area */

GdkPixmap *darea_pxm;	/* backing pixmap for darea */
GdkGC *darea_gc;	/* attached Graphics Context */

/* character information */
int s_i, s_j;		/* coordinates (unit = tile) */
int s_dir;		/* current direction */
int s_step;		/* step in the animation */
int s_push;		/* indicates if he is pushing a box */
int s_undo;             /* traditionnal move or undo */

History hist[HIST_SIZE];  /* history for undo */
int s_hist;               /* current index for history */

/* pixmaps and their masks : [dir][step][pixmap/mask] */
GdkPixbuf *s_pxm[5][5];
GdkPixbuf *ps_pxm[5][5];

/* tiles */
GdkPixbuf *wall_pxm;
GdkPixbuf *tile_pxm;
GdkPixbuf *tile2_pxm;
GdkPixbuf *box_pxm;
GdkPixbuf *box2_pxm;

/* indicates the offset[dir][X/Y] resulting from a movement */
extern const int mv_result[5][2];

/* current level */
int level;

/* number of moves already done */
int nmoves;

/* level data */
char board[W_BOARD][H_BOARD];
int nbox;	/* box counter */

/*------------------------------------*/
/* Functions prototypes               */
/*------------------------------------*/

/* load the pixmaps */
void load_pixmaps(void);

/* initialize globals */
void init_var(void);

/* init level from $HOME/.gsokorc */
void init_level(void);

/* save level in file HOME/.gsokorc */
void save_level(void);

/* start a new game */
void new_game(void);

/* restart at current level */
void restart(void);

/* load (and draw) the nth level */
void load_level(int n);

/* go to next level */
void next_level(void);

/* build the menubar */
void get_main_menu(GtkWidget *window, GtkWidget **menubar);

/* redraw darea (from pixmap) */
int expose_event(GtkWidget *widget, GdkEventExpose *event);

/* handle keypresses over the window */
void key_press_event(GtkWidget *widget, GdkEventKey *event);

/* terminate the application */
void goodbye(void);

/* call goodbye */
int delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);

/* display the 'About' dialog */
void about(void);

/* display a dialog when all levels are completed */
void udidit(void);

/* updates the title of the main window */
void make_title(void);

/* callback for the animation timeout */
int move_callback(gpointer data);

/* callback for the undo */
void undo_move(void);

/* indicate that we want to move towards dir */
void move_s (int dir);

/* draw the character onto pixmap (return the area to refresh) */
GdkRectangle draw_s (void);

/* erase the character from pixmap (return the area to refresh) */
GdkRectangle erase_s (void);

/* refresh darea */
void refresh_darea (GdkRectangle rect);

/* restore the background */
void restore_bg (GdkRectangle rect);

/* draw one tile */
void draw_tile (int i, int j);

/* return TRUE if we can move from (istart,jstart) towards dir, FALSE else */
int is_mvt(int i, int j, int dir);

#endif

