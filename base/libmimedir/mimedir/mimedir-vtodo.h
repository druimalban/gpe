/* RFC 2445 vTodo MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vtodo.h 176 2005-02-26 22:46:04Z srittau $
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#ifndef __MIMEDIR_VTODO_H__
#define __MIMEDIR_VTODO_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


#define MIMEDIR_TYPE_VTODO		(mimedir_vtodo_get_type())
#define MIMEDIR_VTODO(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VTODO, MIMEDirVTodo))
#define MIMEDIR_VTODO_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VTODO, MIMEDirVTodoClass))
#define MIMEDIR_IS_VTODO(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VTODO))
#define MIMEDIR_IS_VTODO_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VTODO))

typedef struct _MIMEDirVTodo		MIMEDirVTodo;
typedef struct _MIMEDirVTodoClass	MIMEDirVTodoClass;
typedef struct _MIMEDirVTodoPriv	MIMEDirVTodoPriv;

struct _MIMEDirVTodo
{
	MIMEDirVComponent parent;

	MIMEDirVTodoPriv *priv;
};

struct _MIMEDirVTodoClass
{
	MIMEDirVComponentClass parent_class;
};


GType		 mimedir_vtodo_get_type			(void);
MIMEDirVTodo	*mimedir_vtodo_new			(void);
MIMEDirVTodo	*mimedir_vtodo_new_from_profile		(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_vtodo_read_from_profile	(MIMEDirVTodo *vtodo, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vtodo_write_to_profile		(MIMEDirVTodo *vtodo);
gboolean	 mimedir_vtodo_write_to_channel		(MIMEDirVTodo *vtodo, GIOChannel *channel, GError **error);
gchar		*mimedir_vtodo_write_to_string		(MIMEDirVTodo *vtodo);

#endif /* __MIMEDIR_VTODO_H__ */
