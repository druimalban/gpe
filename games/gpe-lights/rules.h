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
 * the rules declaration file
 */

#ifndef _RULES_H_
#define _RULES_H_

#include "interface.h"
#include "event.h"

void create_rules(interface *, int);
void delete_rules(interface *);
int rules_redraw(interface *);
int rules_click(interface *, event *);
int rules_move(interface *, event *);
#endif /* _RULES_H_ */
