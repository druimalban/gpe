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
 * the rules file
 */

#include "rules.h"

#include <X11/Xlib.h>

#include "interface.h"
#include "event.h"

static char *_rules =
"Welcome to XLightOff !\n\n"
"The goal of the game is simple :\n"
"switch all the lights off.\n\n"
"To play, just click on the board.\n"
"You will switch on/off some lights.\n"
"When all is off, you win !\n"
"I hope I was clear...\n"
"Anyway, just try it,\n"
"you will understand the rules.\n"
"Start at level 1.\n"
"\n*--------\n*The menu\n*--------\n"
"*prev, next : to jump to previous/next level\n"
"*quit : to quit\n"
"*solve : for the game to show you the shortest solution\n"
"*        (click again to hide the solution)\n"
"*reset : to reset the current level\n"
"*random : for a random level\n"
"*rules : to get this window\n"
"\nEnjoy !\n";

static char *_well_done =
"Congratulations !\n\n"
"You have done all the levels !\n"
"(unless you cheated :)\n"
"If you found a nice method of\n"
"resolution, send it to sed@free.fr\n"
"for it to be stated in the homepage :\n"
"http://sed.free.fr/xlightoff\n"
"Some methods already exist, but your own may be\n"
"better or original in a way or another... :-)\n"
"Enjoy your life.\n";

static char *_next_level =
"Well done !\n"
"Let's jump to the next level.\n";

static char *_random_level =
"Yes !\n"
"You did it !\n"
"The random level !!!\n"
"Let's go back to the normal levels...\n";

static char *rules;

static int rules_maxwidth;
static int rules_nb_lines;
static int bx, by;
static int button_mode = 0;

static inline void get_rules_geometry(void)
{
  int i;
  int cw = 0;

  rules_maxwidth = 0;
  rules_nb_lines = 0;

  for (i=0; rules[i]; i++) {
    if (rules[i] == '*')
      continue;

    if (rules[i] == '\n') {
      rules_nb_lines++;
      if (cw > rules_maxwidth) rules_maxwidth = cw;
      cw = 0;
    } else
      cw++;
  }

  bx = 20 + 6 * rules_maxwidth;
  bx -= BW;
  bx >>= 1;

  by = 20 + 20 + 14 * rules_nb_lines;
}

void create_rules(interface *it, int what)
{
  if (it->rules_on)
    return;

  if (what == 0)
    rules = _rules;
  else if (what == 1)
    rules = _well_done;
  else if (what == 2)
    rules = _next_level;
  else
    rules = _random_level;

  get_rules_geometry();

  it->rw = XCreateSimpleWindow(it->d, DefaultRootWindow(it->d), 0, 0,
				20 + 6 * rules_maxwidth, 20 + BH+40 + 14 * rules_nb_lines,
				0, BlackPixel(it->d, DefaultScreen(it->d)),
				WhitePixel(it->d, DefaultScreen(it->d)));
  XStoreName(it->d, it->rw, "XLightOff rules");
  XSelectInput(it->d, it->rw, ExposureMask | PointerMotionMask | ButtonPressMask | EnterWindowMask | LeaveWindowMask);
  XMapWindow(it->d, it->rw);
  XSetWMProtocols(it->d, it->rw, &it->wm_delete_window, 1);

  it->rules_on = 1;
}

void delete_rules(interface *it)
{
  if (!it->rules_on)
    return;

  XUnmapWindow(it->d, it->rw);
  XDestroyWindow(it->d, it->rw);

  it->rules_on = 0;
}

int rules_redraw(interface *it)
{
  int x, y=20;
  int i = 0;

  if (!it->rules_on)
    return 0;

  while (rules[i]) {
    /* get line width */
    int lw=0;
    while (rules[i+lw++]!='\n')
      if (rules[i+lw] == 0)
        break;

    x = 20 + rules_maxwidth*6;
    x -= lw*6;
    x >>= 1;

    if (rules[i]=='*') {
      lw--;
      i++;
      x = 10;
    }

    if (lw!=1)
      draw_string_rules(it, x, y+12, &rules[i], lw-1);

    y += 14;
    i += lw;
  }

  draw_button_rules(it, bx, by, ON);

  return 0;
}

#define is_on_ok_button(x, y) (x >= bx && x <= bx + BW && y >= by && y <= by + BH)

int rules_click(interface *it, event *ev)
{
  if (!it->rules_on)
    return 0;

  if (it->rules_on != 2 && !is_on_ok_button(ev->px, ev->py))
    return 0;

  delete_rules(it);

  if (it->state == WIN) {
    do_next(it);
    it->state = PLAY;
  }

  return 0;
}

int rules_move(interface *it, event *ev)
{
  int nmode = is_on_ok_button(ev->px, ev->py);

  if (!it->rules_on)
    return 0;

  if (nmode != button_mode) {
    button_mode = nmode;
    draw_button_rules(it, bx, by, nmode);
  }

  return 0;
}
