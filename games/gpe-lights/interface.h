/*
 * Switch the light off !
 * A little brain-oriented game.
 *-----
 * To My angel.
 * December 2001.
 * Sed
 *-----
 * This file is in the public domain.
 *-----
 * the interface include file
 */

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <X11/Xlib.h>

/* interface state */
#define SOLVE	0
#define PLAY	1
#define WIN	2

/* old school dimensions :) */
#define DEFAULT_WIDTH	320
#define DEFAULT_HEIGHT	200

#define BW	50
#define BH	15

/* width and height of the board */
#define N	5

#define ON	0
#define OFF	1

/* the buttons */
#define BUTTON_NONE	0
#define BUTTON_PREV	1
#define BUTTON_NEXT	2
#define BUTTON_QUIT	3
#define BUTTON_SOLVE	4
#define BUTTON_RESET	5
#define BUTTON_RANDOM	6
#define BUTTON_RULES	7

typedef struct {
  Display *d;
  Window w;
  int ww, wh;
  GC gc;
  Atom wm_delete_window;
  Atom wm_protocols;
  int state;
  long start_time_sec;
  long start_time_usec;
  long run_time;
  int tab[N][N];
  int sol[N][N];
  int show_sol;
  int nb_off;
  int nb_moves;
  int level;
  int top_level;
  int current_button;
  int current_case_x, current_case_y;
  /* rules stuff */
  Window rw;
  int rules_on;
} interface;

int init_interface(interface *);
int close_interface(interface *);
void redraw_interface(interface *);
void draw_button(interface *, int, int);
void circle_case(interface *, int, int, int);
void draw_case(interface *, int, int);
void draw_string_rules(interface *, int, int, char *, int);
void draw_button_rules(interface *, int, int, int);

void draw_time(interface *);
void check_time(interface *);
void draw_nb_moves(interface *);
void draw_level(interface *);
void draw_solution(interface *it);
void clear_solution(interface *it);

#ifdef _INSIDE_INTERFACE_C_

static void draw_string(interface *, int, int, char *);
static void draw_board(interface *);

#endif /* _INSIDE_INTERFACE_C_ */

#endif /* _INTERFACE_H_ */
