/* RFC 2445 vTimeZone MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vtimezone.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VTIMEZONE_H__
#define __MIMEDIR_VTIMEZONE_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


#define MIMEDIR_TYPE_VTIMEZONE			(mimedir_vtimezone_get_type())
#define MIMEDIR_VTIMEZONE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VTIMEZONE, MIMEDirVTimeZone))
#define MIMEDIR_VTIMEZONE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VTIMEZONE, MIMEDirVTimeZoneClass))
#define MIMEDIR_IS_VTIMEZONE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VTIMEZONE))
#define MIMEDIR_IS_VTIMEZONE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VTIMEZONE))

typedef struct _MIMEDirVTimeZone	MIMEDirVTimeZone;
typedef struct _MIMEDirVTimeZoneClass	MIMEDirVTimeZoneClass;
typedef struct _MIMEDirVTimeZonePriv	MIMEDirVTimeZonePriv;

struct _MIMEDirVTimeZone
{
	MIMEDirVComponent parent;

	MIMEDirVTimeZonePriv *priv;
};

struct _MIMEDirVTimeZoneClass
{
	MIMEDirVComponentClass parent_class;
};


GType			 mimedir_vtimezone_get_type		(void);
MIMEDirVTimeZone	*mimedir_vtimezone_new			(void);
MIMEDirVTimeZone	*mimedir_vtimezone_new_from_profile	(MIMEDirProfile *profile, GError **error);

gboolean		 mimedir_vtimezone_read_from_profile	(MIMEDirVTimeZone *vtimezone, MIMEDirProfile *profile, GError **error);
MIMEDirProfile		*mimedir_vtimezone_write_to_profile	(MIMEDirVTimeZone *vtimezone);
gboolean		 mimedir_vtimezone_write_to_channel	(MIMEDirVTimeZone *vtimezone, GIOChannel *channel, GError **error);
gchar			*mimedir_vtimezone_write_to_string	(MIMEDirVTimeZone *vtimezone);

#endif /* __MIMEDIR_VTIMEZONE_H__ */
