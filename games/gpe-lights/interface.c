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
 * the interface file
 */

#define _INSIDE_INTERFACE_C_
#include "interface.h"

#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "event.h"
#include "rules.h"
#include "solve.h"

int init_interface(interface *it)
{
  Atom a;
  XGCValues gcv;
  long sec, usec;

  memset(it, 0, sizeof(*it));

  it->d = XOpenDisplay(0);

  if (!it->d)
    return -1;

  it->ww = DEFAULT_WIDTH;
  it->wh = DEFAULT_HEIGHT;

  it->w = XCreateSimpleWindow(it->d, DefaultRootWindow(it->d), 0, 0, it->ww, it->wh, 0,
			      BlackPixel(it->d, DefaultScreen(it->d)),
			      WhitePixel(it->d, DefaultScreen(it->d)));
  XStoreName(it->d, it->w, "XLightOff 1.1");

  it->gc = XCreateGC(it->d, it->w, 0, &gcv);
  XCopyGC(it->d, DefaultGC(it->d, DefaultScreen(it->d)), -1L, it->gc);

  it->state = PLAY;
  it->current_button = BUTTON_NONE;
  it->current_case_x = -1;
  /* top_level must be defined before we call set_level */
  it->top_level = get_top_level();
  set_level(it, 1);

  XSelectInput(it->d, it->w, ExposureMask | PointerMotionMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask);
  XMapWindow(it->d, it->w);

  /* My angel asked for the delete window facility, here it comes. */
  /* if it fails somewhere, no problem, we simply will ignore WM_DELETE_WINDOW */
  /* will we have troubles with the XSetWMProtocols ? I don't know how it will
   * behave if there are some already defined values inside the server for this
   * window. Under fvwm, there is no value predefined. So, well...
   */
  a=XInternAtom(it->d, "WM_DELETE_WINDOW", True);
  it->wm_delete_window = a;
  it->wm_protocols = XInternAtom(it->d, "WM_PROTOCOLS", True);
  if (a != None)
    XSetWMProtocols(it->d, it->w, &a, 1);

  get_time(&sec, &usec);
  srandom(sec + usec);

  return 0;
}

int close_interface(interface *it)
{
  if (it->rules_on)
    delete_rules(it);
  XFreeGC(it->d, it->gc);
  XUnmapWindow(it->d, it->w);
  XDestroyWindow(it->d, it->w);
  XCloseDisplay(it->d);

  return 0;
}

void redraw_interface(interface *it)
{
  draw_button(it, 1, ON);
  draw_button(it, 2, ON);
  draw_button(it, 3, ON);
  draw_button(it, 4, ON);
  draw_button(it, 5, ON);
  draw_button(it, 6, ON);
  draw_button(it, 7, ON);

  draw_string(it, 10, 150+12, "time :");
  draw_string(it, 10, 170+12, "nb moves :");
  draw_string(it, 150+40, 190, "level :");

  draw_time(it);
  draw_nb_moves(it);
  draw_level(it);

  draw_board(it);
}

void draw_button(interface *it, int button, int onoff)
{
  static int x[] = { 0, 10, 70, 40, 40, 40, 40, 40 };
  static int y[] = { 0, 10, 10, 40, 60, 80, 100, 120 };
  static char *text[] = { 0, "prev", "next", "quit", "solve", "reset", "random", "rules" };
  XGCValues gcv;
  int tlen = strlen(text[button]);
  int px;
  register int X = x[button];
  register int Y = y[button];
  register char *Text = text[button];

  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  if (onoff == OFF) {register unsigned long t; t = c1; c1 = c2; c2 = t;}

  gcv.foreground = c2;
  gcv.background = c1;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XFillRectangle(it->d, it->w, it->gc, X, Y, BW, BH);

  gcv.foreground = c1;
  gcv.background = c2;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XDrawLine(it->d, it->w, it->gc, X, Y, X+BW, Y);
  XDrawLine(it->d, it->w, it->gc, X+BW, Y, X+BW, Y+BH);
  XDrawLine(it->d, it->w, it->gc, X+BW, Y+BH, X, Y+BH);
  XDrawLine(it->d, it->w, it->gc, X, Y+BH, X, Y);

  px = BW - tlen * 6;
  px >>= 1;
  XDrawString(it->d, it->w, it->gc, X+px, Y+12, Text, tlen);

  gcv.foreground = BlackPixel(it->d, DefaultScreen(it->d));
  gcv.background = WhitePixel(it->d, DefaultScreen(it->d));
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

void draw_board(interface *it)
{
  int i, j;

  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      XDrawArc(it->d, it->w, it->gc, 2 + 150 + i * 30, 2 + 20 + j * 30, 26, 26, 0, 360*64);
      draw_case(it, j, i);
    }

  draw_solution(it);
}

void clear_solution(interface *it)
{
  int i, j;
  XGCValues gcv;
  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  gcv.foreground = c2;
  gcv.background = c1;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      XDrawLine(it->d, it->w, it->gc, 151+i*30, 21+j*30, 179+i*30, 21+j*30);
      XDrawLine(it->d, it->w, it->gc, 179+i*30, 21+j*30, 179+i*30, 49+j*30);
      XDrawLine(it->d, it->w, it->gc, 179+i*30, 49+j*30, 151+i*30, 49+j*30);
      XDrawLine(it->d, it->w, it->gc, 151+i*30, 49+j*30, 151+i*30, 21+j*30);
    }

  gcv.foreground = c1;
  gcv.background = c2;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

void draw_solution(interface *it)
{
  int i, j;
  XGCValues gcv;
  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  if (!it->show_sol)
    return;

  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      if (it->sol[j][i]) {
	gcv.foreground = c1;
	gcv.background = c2;
      } else {
	gcv.foreground = c2;
	gcv.background = c1;
      }

      XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

      XDrawLine(it->d, it->w, it->gc, 151+i*30, 21+j*30, 179+i*30, 21+j*30);
      XDrawLine(it->d, it->w, it->gc, 179+i*30, 21+j*30, 179+i*30, 49+j*30);
      XDrawLine(it->d, it->w, it->gc, 179+i*30, 49+j*30, 151+i*30, 49+j*30);
      XDrawLine(it->d, it->w, it->gc, 151+i*30, 49+j*30, 151+i*30, 21+j*30);
    }

  gcv.foreground = BlackPixel(it->d, DefaultScreen(it->d));
  gcv.background = WhitePixel(it->d, DefaultScreen(it->d));
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

void draw_string(interface *it, int x, int y, char *s)
{
  int slen = strlen(s);
  XDrawString(it->d, it->w, it->gc, x, y, s, slen);
}

static void draw_dark_string(interface *it, int x, int y, char *s, int n)
{
  char t[21];
  int slen = strlen(s);

  if (n > 20) n = 20;

  memset(t, ' ', n);
  t[n] = 0;

  if (slen > n) slen = n;
  memcpy(t, s, slen);

  XDrawImageString(it->d, it->w, it->gc, x, y, t, n);
}

void circle_case(interface *it, int x, int y, int onoff)
{
  XGCValues gcv;

  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  if (onoff == OFF) {register unsigned long t; t = c1; c1 = c2; c2 = t;}
  gcv.foreground = c2;
  gcv.background = c1;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XDrawArc(it->d, it->w, it->gc, 3+ 150 + x * 30, 3+ 20 + y * 30, 24, 24, 0, 360*64);

  gcv.foreground = BlackPixel(it->d, DefaultScreen(it->d));
  gcv.background = WhitePixel(it->d, DefaultScreen(it->d));
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

void draw_case(interface *it, int x, int y)
{
  XGCValues gcv;

  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  if (it->tab[y][x]) {register unsigned long t; t = c1; c1 = c2; c2 = t;}
  gcv.foreground = c2;
  gcv.background = c1;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XFillArc(it->d, it->w, it->gc, 5+ 150 + x * 30, 5+ 20 + y * 30, 20, 20, 0, 360*64);

  gcv.foreground = BlackPixel(it->d, DefaultScreen(it->d));
  gcv.background = WhitePixel(it->d, DefaultScreen(it->d));
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

void draw_string_rules(interface *it, int x, int y, char *s, int l)
{
  XDrawString(it->d, it->rw, it->gc, x, y, s, l);
}

void draw_button_rules(interface *it, int X, int Y, int onoff)
{
  register char *Text = "ok";
  XGCValues gcv;
  int tlen = strlen(Text);
  int px;

  unsigned long c1 = BlackPixel(it->d, DefaultScreen(it->d));
  unsigned long c2 = WhitePixel(it->d, DefaultScreen(it->d));

  if (onoff == OFF) {register unsigned long t; t = c1; c1 = c2; c2 = t;}

  gcv.foreground = c2;
  gcv.background = c1;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XFillRectangle(it->d, it->rw, it->gc, X, Y, BW, BH);

  gcv.foreground = c1;
  gcv.background = c2;
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);

  XDrawLine(it->d, it->rw, it->gc, X, Y, X+BW, Y);
  XDrawLine(it->d, it->rw, it->gc, X+BW, Y, X+BW, Y+BH);
  XDrawLine(it->d, it->rw, it->gc, X+BW, Y+BH, X, Y+BH);
  XDrawLine(it->d, it->rw, it->gc, X, Y+BH, X, Y);

  px = BW - tlen * 6;
  px >>= 1;
  XDrawString(it->d, it->rw, it->gc, X+px, Y+12, Text, tlen);

  gcv.foreground = BlackPixel(it->d, DefaultScreen(it->d));
  gcv.background = WhitePixel(it->d, DefaultScreen(it->d));
  XChangeGC(it->d, it->gc, GCForeground | GCBackground, &gcv);
}

/* mode 0 : always display, mode 1 : display only when changes */
static void _draw_time(interface *it, int mode)
{
  char tc[50];
  long t;
  int nb_min, nb_sec;

  if (it->state == PLAY)
    t = diff_time(it);
  else
    t = it->run_time;

  if (mode == 1 && t == it->run_time)
    return;

  it->run_time = t;

  nb_min = t / 60;
  nb_sec = t % 60;

  snprintf(tc, 50, "%d:%2.2d", nb_min, nb_sec);
  draw_dark_string(it, 10+6*7, 150+12, tc, 11);
}

void draw_time(interface *it)
{
  _draw_time(it, 0);
}

void check_time(interface *it)
{
  _draw_time(it, 1);
}

static void draw_number(interface *it, int x, int y, int n)
{
  char s[32];

  snprintf(s, 32, "%d", n);
  draw_dark_string(it, x, y, s, 10);
}

void draw_nb_moves(interface *it)
{
  draw_number(it, 10+6*11, 170+12, it->nb_moves);
}

void draw_level(interface *it)
{
  if (it->level == it->top_level + 1)
    draw_dark_string(it, 150+40+8*6, 190, "random", 6);
  else
    draw_number(it, 150+40+8*6, 190, it->level);
}
