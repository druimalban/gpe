/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef HAVE_DATE_H
#define HAVE_DATE_H

struct {
	GtkWidget *toplevel;
	GtkWidget *label;

	char *label_text;
} date;

void date_init(void);
void date_free(void);
void date_update(void);

#endif /* !HAVE_DATE_H */
