/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef HAVE_TODO_H
#define HAVE_TODO_H

struct {
	GtkWidget *toplevel;

	PangoLayout *message;

	struct myscroll *scroll;
} todo;

void todo_init(void);
void todo_free(void);
gboolean todo_update(gpointer data);

#endif /* !HAVE_TODO_H */
