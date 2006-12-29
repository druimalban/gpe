/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2006 Neal H. Walfield <neal@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef GPE_VTODO_H
#define GPE_VTODO_H

#include <glib.h>
#include <mimedir/mimedir-vtodo.h>

/* Import a TODO item into the todo-db.  Returns whether it
   succeeded.  */
extern gboolean todo_import_from_vtodo (MIMEDirVTodo *todo, GError **error);

/* XXX: Deprecated.  */
#include <gpe/tag-db.h>

extern MIMEDirVTodo *vtodo_from_tags (GSList *tags);
extern GSList *vtodo_to_tags (MIMEDirVTodo *vtodo);

#endif
