/*
 * Copyright (C) 2002 Robert Mibus <mibus@handhelds.org>
 *                    Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef QUESTION_H
#define QUESTION_H

extern gint gpe_question_ask (char *qn, char *title, char *iconname, ...);
extern gint gpe_question_ask_yn (char *qn);

#endif
