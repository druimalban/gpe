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
 * the event include file
 */

#ifndef _EVENT_H_
#define _EVENT_H_

/* event type */
#define EV_NONE		0
#define EV_REDRAW	1
#define EV_TIMEOUT	2
#define	EV_MOVE		3
#define	EV_CLICK	4
#define	EV_KEY		5
#define RULES_EV_REDRAW	6
#define RULES_EV_CLICK	7
#define RULES_EV_MOVE	8
#define EV_QUIT		9
#define RULES_EV_QUIT	10

typedef struct {
  int type;
  int px, py;
  char key;
} event;

#include "interface.h"

int wait_event(event *, interface *);
int handle_event(event *, interface *);
void get_time(long *, long *);
long diff_time(interface *);
long diff_minitime(interface *);
void set_level(interface *, int);
int do_next(interface *);
int get_top_level(void);

#ifdef _IN_EVENT_C_

static int case_action(interface *, event *);
extern int (*button_action[])(interface *);
static int you_win(interface *);
int do_quit(interface *);

#endif /* _IN_EVENT_C_ */

#endif /* _EVENT_H_ */
