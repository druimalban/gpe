/* RFC 2445 Attendee Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attendee.h 104 2002-12-05 22:56:51Z srittau $
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

#ifndef __MIMEDIR_ATTENDEE_H__
#define __MIMEDIR_ATTENDEE_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_TYPE_ATTENDEE			(mimedir_attendee_get_type())
#define MIMEDIR_ATTENDEE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_ATTENDEE, MIMEDirAttendee))
#define MIMEDIR_ATTENDEE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_ATTENDEE, MIMEDirAttendeeClass))
#define MIMEDIR_IS_ATTENDEE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_ATTENDEE))
#define MIMEDIR_IS_ATTENDEE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_ATTENDEE))

typedef struct _MIMEDirAttendee		MIMEDirAttendee;
typedef struct _MIMEDirAttendeeClass	MIMEDirAttendeeClass;
typedef struct _MIMEDirAttendeePriv	MIMEDirAttendeePriv;

struct _MIMEDirAttendee
{
	GObject parent;

	/* Read Only */

	gchar *uri;

	MIMEDirAttendeePriv *priv;
};

struct _MIMEDirAttendeeClass
{
	GObjectClass parent_class;
};


GType		 mimedir_attendee_get_type		(void);

MIMEDirAttendee	*mimedir_attendee_new			(void);
MIMEDirAttendee	*mimedir_attendee_new_from_attribute	(MIMEDirAttribute *attribute, GError **error);

gboolean	 mimedir_attendee_read_from_attribute	(MIMEDirAttendee *attendee, MIMEDirAttribute *attribute, GError **error);
MIMEDirAttribute*
		 mimedir_attendee_write_to_attribute	(MIMEDirAttendee *attendee);

#endif /* __MIMEDIR_ATTENDEE_H__ */
