/* RFC 2445 vEvent MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vevent.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VEVENT_H__
#define __MIMEDIR_VEVENT_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


#define MIMEDIR_TYPE_VEVENT		(mimedir_vevent_get_type())
#define MIMEDIR_VEVENT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VEVENT, MIMEDirVEvent))
#define MIMEDIR_VEVENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VEVENT, MIMEDirVEventClass))
#define MIMEDIR_IS_VEVENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VEVENT))
#define MIMEDIR_IS_VEVENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VEVENT))

typedef struct _MIMEDirVEvent		MIMEDirVEvent;
typedef struct _MIMEDirVEventClass	MIMEDirVEventClass;
typedef struct _MIMEDirVEventPriv	MIMEDirVEventPriv;

struct _MIMEDirVEvent
{
	MIMEDirVComponent parent;

	MIMEDirVEventPriv *priv;
};

struct _MIMEDirVEventClass
{
	MIMEDirVComponentClass parent_class;
};


GType		 mimedir_vevent_get_type		(void);
MIMEDirVEvent	*mimedir_vevent_new			(void);
MIMEDirVEvent	*mimedir_vevent_new_from_profile	(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_vevent_read_from_profile	(MIMEDirVEvent *vevent, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vevent_write_to_profile	(MIMEDirVEvent *vevent);
gboolean	 mimedir_vevent_write_to_channel	(MIMEDirVEvent *vevent, GIOChannel *channel, GError **error);
gchar		*mimedir_vevent_write_to_string		(MIMEDirVEvent *vevent);

#endif /* __MIMEDIR_VEVENT_H__ */
