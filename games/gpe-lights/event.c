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
 * the event file
 */

#define _IN_EVENT_C_
#include "event.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "rules.h"
#include "solve.h"

/* a predicate function returning always true. X wants this */
static Bool true(Display *d, XEvent *ev, XPointer t)
{
  return True;
}

int wait_event(event *ev, interface *it)
{
  XEvent xev;
  fd_set rd;
  struct timeval to;
  register int d = ConnectionNumber(it->d);

wait_event_start:
  if (XCheckIfEvent(it->d, &xev, true, 0))
    goto screen_event;

  FD_ZERO(&rd);
  FD_SET(d, &rd);

  if (it->state == PLAY) {
    long s, us;

    get_time(&s, &us);

    if (us > it->start_time_usec)
      to.tv_usec = 1000000 + it->start_time_usec - us;
    else
      to.tv_usec = it->start_time_usec - us;

    to.tv_sec = 0;

    if (select (d+1, &rd, 0, 0, &to) == -1)
      return -1;
  } else
    if (select (d+1, &rd, 0, 0, 0) == -1)
      return -1;

  if (!FD_ISSET(d, &rd)) {
    ev->type = EV_TIMEOUT;
    return 0;
  }

  XNextEvent(it->d, &xev);
screen_event:

  switch (xev.type) {
    case Expose :
      /* we must remove all the Expose from the events' queue */
      while (XCheckWindowEvent(it->d, xev.xexpose.window, ExposureMask, &xev))
	/* do nothing */;
      if (xev.xexpose.window == it->w)
	ev->type = EV_REDRAW;
      else
	ev->type = RULES_EV_REDRAW;
      break;
    case MotionNotify :
      if (xev.xmotion.window == it->w)
	ev->type = EV_MOVE;
      else
	ev->type = RULES_EV_MOVE;
      ev->px = xev.xmotion.x;
      ev->py = xev.xmotion.y;
      break;
    case ButtonPress :
      if (xev.xbutton.button != Button1)
	goto wait_event_start;
      if (xev.xbutton.window == it->w)
	ev->type = EV_CLICK;
      else
	ev->type = RULES_EV_CLICK;
      ev->px = xev.xbutton.x;
      ev->py = xev.xbutton.y;
      break;
    case EnterNotify :
      if (xev.xcrossing.window == it->w)
	ev->type = EV_MOVE;
      else
	ev->type = RULES_EV_MOVE;
      ev->px = xev.xcrossing.x;
      ev->py = xev.xcrossing.y;
      break;
    case LeaveNotify :
      if (xev.xcrossing.window == it->w)
	ev->type = EV_MOVE;
      else
	ev->type = RULES_EV_MOVE;
      ev->px = -1;
      ev->py = -1;
      break;
    case ClientMessage :
      if (xev.xclient.message_type != it->wm_protocols)
	goto wait_event_start;
      if (xev.xclient.data.l[0] != it->wm_delete_window)
	goto wait_event_start;
      if (xev.xclient.window == it->w) {
	ev->type = EV_QUIT;
	break;
      }
      if (xev.xclient.window == it->rw) {
	ev->type = RULES_EV_QUIT;
	break;
      }
      goto wait_event_start;
  }

  if (it->state == WIN) {
    if (ev->type == EV_CLICK || ev->type == EV_KEY)
      XBell(it->d, 10);
    if ((ev->type == EV_MOVE && ev->px!=-1) ||
	ev->type == EV_CLICK ||
	ev->type == EV_KEY)
      goto wait_event_start;
  }

  return 0;
}

#define is_on_board(x, y) (x >= 150 && x < 300 && y >= 20 && y < 170)

static void get_case_on_board(int *x, int *y, int sx, int sy)
{
  int lx = (sx - 150) / 30;
  int ly = (sy - 20) / 30;
  int cx, cy;

  if (!is_on_board(sx, sy)) {
    *x = -1;
    return;
  }

  cx = 150 + 30 * lx + 15;
  cy = 20 + 30 * ly + 15;
  cx -= sx;
  cy -= sy;
  cx *= cx;
  cy *= cy;
  cx += cy;
  if (cx>13*13) {
    *x = -1;
    return;
  }

  *x = lx;
  *y = ly;
}

static int move_on_board(event *ev, interface *it)
{
  int x;
  int y;

  get_case_on_board(&x, &y, ev->px, ev->py);

  if (x == -1) {
    if (it->current_case_x != -1) {
      circle_case(it, it->current_case_x, it->current_case_y, ON);
      it->current_case_x = -1;
    }
    return 0;
  }

  if (x == it->current_case_x && y == it->current_case_y)
    return 0;

  circle_case(it, it->current_case_x, it->current_case_y, ON);
  circle_case(it, x, y, OFF);

  it->current_case_x = x;
  it->current_case_y = y;

  return 0;
}

static int which_button(int x, int y)
{
  if (x<10 || x>70+BW || y<10 || y>120+BW)
    return -1;

  if (y >= 10 && y <= 10+BH) {
    if (x>=10 && x<=10+BW)
      return BUTTON_PREV;
    if (x>=70 && x<=70+BW)
      return BUTTON_NEXT;
    return -1;
  }

  if (x >= 40 && x <= 40+BW) {
    if (y>=40 && y <=40+BH)
      return BUTTON_QUIT;
    if (y>=60 && y <=60+BH)
      return BUTTON_SOLVE;
    if (y>=80 && y <=80+BH)
      return BUTTON_RESET;
    if (y>=100 && y <=100+BH)
      return BUTTON_RANDOM;
    if (y>=120 && y <=120+BH)
      return BUTTON_RULES;
    return -1;
  }

  return -1;
}

int handle_event(event *ev, interface *it)
{
  int button;

  if (ev->type != EV_TIMEOUT && it->state == PLAY)
    check_time(it);

  switch (ev->type) {
    case EV_TIMEOUT :
      draw_time(it);
      return 0;

    case EV_REDRAW :
      redraw_interface(it);
      return 0;

    case EV_MOVE :
      button = which_button(ev->px, ev->py);
      if (button == -1) {
	if (it->current_button) {
	  draw_button(it, it->current_button, ON);
	  it->current_button = BUTTON_NONE;
	}

	return move_on_board(ev, it);
      }

      if (it->current_case_x != -1) { 
	circle_case(it, it->current_case_x, it->current_case_y, ON);
	it->current_case_x = -1;
      }

      if (button == it->current_button)
	return 0;

      if (it->current_button)
	draw_button(it, it->current_button, ON);

      draw_button(it, button, OFF);
      it->current_button = button;

      return 0;

    case EV_CLICK :
      button = which_button(ev->px, ev->py);
      if (button != -1)
	return button_action[button](it);

      if (is_on_board(ev->px, ev->py))
	return case_action(it, ev);

    case RULES_EV_REDRAW :
      return rules_redraw(it);

    case RULES_EV_CLICK :
      return rules_click(it, ev);

    case RULES_EV_MOVE :
      return rules_move(it, ev);

    case EV_QUIT :
      return do_quit(it);

    case RULES_EV_QUIT :
      if (!it->rules_on)
	return 0;
      it->rules_on = 2; 	/* what a lovely hack :) */
      return rules_click(it, ev);
  }

  return 0;
}

static long sub_time(long s1, long u1, long s2, long u2)
{
  s2 -= s1;
  if (u2 < u1)
    s2--;
  return s2;
}

long diff_time(interface *it)
{
  struct timeval tv;
  struct timezone tz;

  if (gettimeofday(&tv, &tz) == -1)
    return -1L;

  return sub_time(it->start_time_sec, it->start_time_usec, tv.tv_sec, tv.tv_usec);
}

void get_time(long *s, long *us)
{
  struct timeval tv;
  struct timezone tz;

  if (gettimeofday(&tv, &tz) == -1) {
    *s = 0;
    *us = 0;
    return;
  }

  *s = tv.tv_sec;
  *us = tv.tv_usec;
}

static int case_action(interface *it, event *ev)
{
  int x, y;

  get_case_on_board(&x, &y, ev->px, ev->py);

  if (x == -1)
    return 0;

  it->nb_moves++;
  draw_nb_moves(it);

  it->tab[y][x] ^= 1;
  draw_case(it, x, y);
  it->nb_off -= it->tab[y][x]*2-1;

  if (x>0) {
    it->tab[y][x-1] ^= 1;
    draw_case(it, x-1, y);
    it->nb_off -= it->tab[y][x-1]*2-1;
  }

  if (y>0) {
    it->tab[y-1][x] ^= 1;
    draw_case(it, x, y-1);
    it->nb_off -= it->tab[y-1][x]*2-1;
  }

  if (x<N-1) {
    it->tab[y][x+1] ^= 1;
    draw_case(it, x+1, y);
    it->nb_off -= it->tab[y][x+1]*2-1;
  }

  if (y<N-1) {
    it->tab[y+1][x] ^= 1;
    draw_case(it, x, y+1);
    it->nb_off -= it->tab[y+1][x]*2-1;
  }

  if (it->show_sol) {
    if (it->sol[y][x])
      it->sol[y][x] = 0;
    else
      solve(it);
    draw_solution(it);
  }

  if (it->nb_off == 25)
    return you_win(it);

  return 0;
}

int do_prev(interface *it)
{
  int l = it->level - 1;

  if (l==0)
    l=it->top_level;

  if (it->level == it->top_level + 1)
    l = 1;
    
  set_level(it, l);

  return 0;
}

int do_next(interface *it)
{
  int l = it->level;

  if (l>=it->top_level)
    l=0;

  l++;

  set_level(it, l);

  return 0;
}

int do_quit(interface *it)
{
  return 1;
}

int do_solve(interface *it)
{
  it->show_sol ^= 1;

  if (it->show_sol) {
    solve(it);
    draw_solution(it);
  } else
    clear_solution(it);

  return 0;
}

int do_reset(interface *it)
{
  set_level(it, it->level + (it->level == it->top_level + 1));

  return 0;
}

int do_random(interface *it)
{
  set_level(it, it->top_level + 1);

  return 0;
}

int do_rules(interface *it)
{
  if (it->rules_on)
    return 0;
  create_rules(it, 0);

  return 0;
}

int (*button_action[])(interface *) = {
  0,
  do_prev,
  do_next,
  do_quit,
  do_solve,
  do_reset,
  do_random,
  do_rules
};

static int you_win(interface *it)
{
  int l = it->level;
  int t = 1;

  if (it->rules_on)
    delete_rules(it);

  it->state = WIN;

  if (l != it->top_level)
    t++;

  if (l == it->top_level + 1)
    t++;

  create_rules(it, t);

  return 0;
}

static int lev[][N][N] = {
/* 1 */
{{1,1,0,1,1},
 {1,0,1,0,1},
 {0,1,1,1,0},
 {1,0,1,0,1},
 {1,1,0,1,1}},

{{0,1,0,1,0},
 {1,1,0,1,1},
 {0,1,0,1,0},
 {1,0,1,0,1},
 {1,0,1,0,1}},

{{1,0,0,0,1},
 {1,1,0,1,1},
 {0,0,1,0,0},
 {1,0,1,0,0},
 {1,0,1,1,0}},

{{1,1,0,1,1},
 {0,0,0,0,0},
 {1,1,0,1,1},
 {0,0,0,0,1},
 {1,1,0,0,0}},
/* 5 */
{{1,1,1,1,1},
 {1,1,1,1,1},
 {1,1,1,1,1},
 {1,1,1,1,1},
 {1,1,1,1,1}},
/* 6 */
{{0,0,0,1,1},
 {0,0,0,1,1},
 {0,0,0,0,0},
 {1,1,0,0,0},
 {1,1,0,0,0}},
/* 7 */
{{0,0,0,0,0},
 {0,1,1,1,0},
 {1,1,1,1,1},
 {0,1,1,1,0},
 {0,0,0,0,0}},
/* 8 */
{{0,0,0,0,0},
 {0,1,1,1,0},
 {0,1,1,1,0},
 {0,1,1,1,0},
 {0,0,0,0,0}},
/* 9 */
{{1,1,0,1,1},
 {1,1,0,1,1},
 {0,0,0,0,0},
 {1,1,0,1,1},
 {1,1,0,1,1}},
/* 10 */
{{1,1,1,1,1},
 {0,1,1,1,0},
 {0,0,1,0,0},
 {0,1,1,1,0},
 {1,1,1,1,1}},
/* 11 */
{{1,1,1,1,1},
 {1,0,0,0,1},
 {1,0,0,0,1},
 {1,0,0,0,1},
 {1,1,1,1,1}},
/* 12 */
{{0,0,1,1,1},
 {0,0,0,1,1},
 {1,0,0,0,1},
 {1,1,0,0,0},
 {1,1,1,0,0}},
/* 13 */
{{1,0,0,0,1},
 {0,1,0,1,0},
 {0,0,1,0,0},
 {0,1,0,1,0},
 {1,0,0,0,1}},
/* 14 */
{{1,1,1,1,1},
 {1,0,1,0,1},
 {1,1,1,1,1},
 {1,0,1,0,1},
 {1,1,1,1,1}},
/* 15 */
{{1,0,0,0,0},
 {1,1,1,1,0},
 {1,1,1,1,0},
 {1,1,1,1,0},
 {1,1,1,1,1}},
/* 16 */
{{1,1,1,1,1},
 {1,1,1,1,1},
 {1,1,0,1,1},
 {1,1,1,1,1},
 {1,1,1,1,1}},
/* 17 */
{{1,0,1,0,1},
 {0,1,0,1,0},
 {0,0,1,0,0},
 {0,1,0,1,0},
 {1,0,1,0,1}},
/* 18 */
{{1,1,1,0,1},
 {1,1,1,0,1},
 {0,0,0,0,0},
 {1,0,1,1,1},
 {1,0,1,1,1}},
/* 19 */
{{1,1,0,1,1},
 {1,1,0,1,1},
 {1,1,0,1,1},
 {1,1,0,1,1},
 {1,1,0,1,1}},
/* 20 */
{{1,1,1,1,1},
 {1,0,0,0,1},
 {1,1,0,1,1},
 {1,1,0,1,1},
 {1,1,1,1,1}},
/* 21 */
{{1,1,1,1,1},
 {1,1,1,1,1},
 {0,0,0,1,1},
 {0,0,0,1,1},
 {0,0,0,1,1}},
/* 22 */
{{1,1,1,0,1},
 {1,1,1,0,0},
 {1,1,1,0,0},
 {1,1,1,0,0},
 {1,1,1,0,1}},
/* 23 */
{{1,1,1,1,1},
 {0,0,1,0,0},
 {0,0,1,0,0},
 {0,0,1,0,0},
 {1,1,1,1,1}},
/* 24 */
{{0,0,0,0,0},
 {0,1,1,0,0},
 {0,1,1,1,0},
 {0,0,1,1,0},
 {0,0,0,0,0}},
/* 25 */
{{0,0,0,1,1},
 {0,0,1,1,1},
 {0,0,1,0,0},
 {1,1,1,0,0},
 {1,1,0,0,0}},
/* 26 */
{{0,0,0,1,1},
 {1,1,0,1,1},
 {1,1,0,0,0},
 {1,1,0,0,0},
 {1,1,0,1,1}},
/* 27 */
{{1,0,0,0,1},
 {0,1,1,1,0},
 {0,1,1,1,0},
 {0,1,1,1,0},
 {1,0,0,0,1}},
/* 28 */
{{1,0,1,0,1},
 {0,1,0,1,0},
 {1,0,1,0,1},
 {0,1,0,1,0},
 {1,0,1,0,1}},
/* 29 */
{{0,0,1,0,0},
 {1,0,0,1,0},
 {0,1,1,1,1},
 {1,0,0,0,0},
 {1,1,0,1,0}},
/* 30 */
{{0,0,0,0,1},
 {0,0,0,1,1},
 {0,0,1,0,1},
 {0,1,0,0,1},
 {1,1,1,1,1}},
/* 31 */
{{1,1,0,1,1},
 {0,1,0,1,0},
 {1,1,1,1,1},
 {1,1,0,1,1},
 {1,0,0,0,1}},
/* 32 */
{{0,1,1,0,0},
 {0,1,1,0,1},
 {0,1,0,0,1},
 {1,1,0,0,0},
 {1,1,1,1,0}},
/* 33 */
{{0,0,0,0,1},
 {1,1,1,0,0},
 {1,0,1,1,1},
 {1,1,1,1,0},
 {1,0,0,1,0}},
/* 34 */
{{1,0,1,1,1},
 {0,0,1,0,1},
 {0,0,0,0,0},
 {1,1,1,1,0},
 {1,1,0,1,0}},
/* 35 */
{{1,1,0,1,1},
 {0,1,0,1,1},
 {0,0,0,1,0},
 {1,1,0,0,0},
 {1,1,1,1,0}},
/* 36 */
{{1,1,1,1,1},
 {0,0,0,1,0},
 {0,1,0,1,1},
 {1,1,1,0,1},
 {1,0,1,0,0}},
/* 37 */
{{0,0,0,1,1},
 {1,0,1,1,0},
 {0,0,1,0,0},
 {0,1,1,1,1},
 {1,0,0,1,0}},
/* 38 */
{{0,0,0,0,1},
 {0,0,1,1,1},
 {1,1,0,0,1},
 {1,1,1,0,0},
 {0,0,1,0,0}},
/* 39 */
{{0,0,1,1,1},
 {1,0,1,1,1},
 {1,1,1,0,0},
 {0,0,1,0,0},
 {1,1,0,1,1}},
/* 40 */
{{0,1,1,1,1},
 {0,0,1,1,1},
 {0,0,1,1,1},
 {1,1,1,1,0},
 {0,0,0,1,0}},
/* 41 */
{{1,1,1,1,1},
 {1,0,0,0,0},
 {0,1,0,0,1},
 {0,0,0,1,1},
 {1,1,1,1,1}},
/* 42 */
{{1,1,1,1,1},
 {1,0,0,0,0},
 {0,0,1,0,0},
 {0,1,1,1,0},
 {0,1,0,0,1}},
/* 43 */
{{0,0,0,0,0},
 {0,0,0,1,0},
 {1,1,0,1,1},
 {0,1,1,0,0},
 {1,1,1,1,1}},
/* 44 */
{{0,0,0,1,1},
 {0,1,1,0,0},
 {0,1,0,0,0},
 {1,1,1,1,0},
 {1,1,1,1,0}},
/* 45 */
{{0,0,0,1,0},
 {1,1,1,1,1},
 {0,0,0,0,0},
 {0,0,1,0,0},
 {1,1,1,1,0}},
/* 46 */
{{0,1,0,1,0},
 {0,0,0,1,0},
 {0,1,0,1,0},
 {0,0,1,0,0},
 {0,1,1,1,0}},
/* 47 */
{{1,0,0,1,0},
 {0,0,0,0,1},
 {0,1,0,0,0},
 {0,0,0,0,0},
 {1,0,1,0,0}},
/* 48 */
{{1,1,0,0,1},
 {0,1,0,0,1},
 {0,1,1,1,1},
 {0,1,0,1,0},
 {1,1,1,1,1}},
/* 49 */
{{1,1,1,1,1},
 {0,0,1,0,0},
 {0,1,1,0,0},
 {0,0,1,1,0},
 {1,1,1,0,1}},
/* 50 */
{{1,0,1,0,1},
 {1,0,1,0,0},
 {0,0,0,1,1},
 {0,1,0,1,0},
 {1,0,0,1,0}},
/* 51 */
{{0,1,0,1,0},
 {1,0,0,1,0},
 {0,1,1,1,1},
 {1,0,1,0,0},
 {0,1,1,0,0}},
/* 52 */
{{1,1,1,1,1},
 {1,1,0,0,0},
 {0,0,0,1,1},
 {0,1,1,1,0},
 {0,0,1,0,0}},
/* random */
{{2,0,0,0,0},
 {0,0,0,0,0},
 {0,0,0,0,0},
 {0,0,0,0,0},
 {0,0,0,0,0}}
};

/* giving level top_level+1 means starting random level
 * giving level top_level+2 means reset random level
 */
void set_level(interface *it, int level)
{
  int i, j;

  get_time(&it->start_time_sec, &it->start_time_usec);

  it->run_time = 0;
  it->level = level;
  it->nb_moves = 0;
  it->show_sol = 0;

  /* random level */
  if (level == it->top_level + 1) {
    int did_random=0;

    /* Clear the board */
    for (i=0; i<N; i++)
      for (j=0; j<N; j++)
	lev[level-1][i][j] = 0;

    /* Hit some button randomly */
    for (i=0; i<N; i++) {
      for (j=0; j<N; j++) {
	if (random() & 1) {
          did_random = 1;
	  /* Press this button */
	  lev[level-1][i][j] = 1 - lev[level-1][i][j];
	  if (i>0)   lev[level-1][i-1][j] = 1-lev[level-1][i-1][j];
	  if (i<N-1) lev[level-1][i+1][j] = 1-lev[level-1][i+1][j];
	  if (j>0)   lev[level-1][i][j-1] = 1-lev[level-1][i][j-1];
	  if (j<N-1) lev[level-1][i][j+1] = 1-lev[level-1][i][j+1];
	}
      }
    }

    /* if did_random = 0, set the center to 1 (it can be solved) so there
     * there is something to solve :) */
    if (!did_random)
      lev[level-1][2][2] = 1;
  }
  if (level == it->top_level + 2)
    level--, it->level--;

  memcpy(it->tab, lev[level-1], N*N*sizeof(int));

  draw_level(it);
  draw_nb_moves(it);
  draw_time(it);

  it->nb_off = 25;

  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      if (it->tab[i][j])
        it->nb_off--;
      draw_case(it, j, i);
  }

  clear_solution(it);
}

int get_top_level(void)
{
  int i;
  for (i=0;; i++)
    if (lev[i][0][0] == 2)
      return i;

  return i; /* for gcc to be happy */
}
