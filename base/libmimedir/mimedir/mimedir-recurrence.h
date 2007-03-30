/* RFC 2445 iCal Recurrence Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-recurrence.h 213 2005-09-01 15:36:27Z srittau $
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

#ifndef __MIMEDIR_RECURRENCE_H__
#define __MIMEDIR_RECURRENCE_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_TYPE_RECURRENCE			(mimedir_recurrence_get_type())
#define MIMEDIR_RECURRENCE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_RECURRENCE, MIMEDirRecurrence))
#define MIMEDIR_RECURRENCE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_RECURRENCE, MIMEDirRecurrenceClass))
#define MIMEDIR_IS_RECURRENCE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_RECURRENCE))
#define MIMEDIR_IS_RECURRENCE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_RECURRENCE))

typedef struct _MIMEDirRecurrence	MIMEDirRecurrence;
typedef struct _MIMEDirRecurrenceClass	MIMEDirRecurrenceClass;
typedef struct _MIMEDirRecurrencePriv	MIMEDirRecurrencePriv;

struct _MIMEDirRecurrence
{
	GObject parent;

	MIMEDirRecurrencePriv *priv;
};

struct _MIMEDirRecurrenceClass
{
	GObjectClass parent_class;
};


typedef enum {
	RECURRENCE_SECONDLY = 1,
	RECURRENCE_MINUTELY,
	RECURRENCE_HOURLY,
	RECURRENCE_DAILY,
	RECURRENCE_WEEKLY,
	RECURRENCE_MONTHLY,
	RECURRENCE_YEARLY,
} MIMEDirRecurrenceFrequency;

typedef enum {
	RECURRENCE_UNIT_NONE = 1,
	RECURRENCE_UNIT_SECOND,
	RECURRENCE_UNIT_MINUTE,
	RECURRENCE_UNIT_HOUR,
	RECURRENCE_UNIT_DAY,
	RECURRENCE_UNIT_MONTHDAY,
	RECURRENCE_UNIT_YEARDAY,
	RECURRENCE_UNIT_WEEKNO,
	RECURRENCE_UNIT_MONTH,
	//FIXME: not handled yet RECURRENCE_UNIT_SETPOS,
} MIMEDirRecurrenceUnit;


GType			 mimedir_recurrence_get_type		(void);

MIMEDirRecurrence	*mimedir_recurrence_new			(void);
MIMEDirRecurrence	*mimedir_recurrence_new_parse		(MIMEDirAttribute *attr, GError **error);

gboolean		 mimedir_recurrence_parse		(MIMEDirRecurrence *recurrence, MIMEDirAttribute *attr, GError **error);

gchar			*mimedir_recurrence_write_to_string	(MIMEDirRecurrence *recurrence);
#endif /* __MIMEDIR_RECURRENCE_H__ */
