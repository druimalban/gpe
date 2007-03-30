/* RFC 2445 vAlarm MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-valarm.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VALARM_H__
#define __MIMEDIR_VALARM_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


typedef enum {
	MIMEDIR_VALARM_UNKNOWN,
	MIMEDIR_VALARM_AUDIO,
	MIMEDIR_VALARM_DISPLAY,
	MIMEDIR_VALARM_EMAIL,
	MIMEDIR_VALARM_PROCEDURE
} MIMEDirVAlarmType;


#define MIMEDIR_TYPE_VALARM		(mimedir_valarm_get_type())
#define MIMEDIR_VALARM(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VALARM, MIMEDirVAlarm))
#define MIMEDIR_VALARM_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VALARM, MIMEDirVAlarmClass))
#define MIMEDIR_IS_VALARM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VALARM))
#define MIMEDIR_IS_VALARM_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VALARM))

typedef struct _MIMEDirVAlarm		MIMEDirVAlarm;
typedef struct _MIMEDirVAlarmClass	MIMEDirVAlarmClass;
typedef struct _MIMEDirVAlarmPriv	MIMEDirVAlarmPriv;

struct _MIMEDirVAlarm
{
	MIMEDirVComponent parent;

	MIMEDirVAlarmPriv *priv;
};

struct _MIMEDirVAlarmClass
{
	MIMEDirVComponentClass parent_class;
};


GType		 mimedir_valarm_get_type		(void);
MIMEDirVAlarm	*mimedir_valarm_new			(MIMEDirVAlarmType type);
MIMEDirVAlarm	*mimedir_valarm_new_from_profile	(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_valarm_read_from_profile	(MIMEDirVAlarm *valarm, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_valarm_write_to_profile	(MIMEDirVAlarm *valarm);
gboolean	 mimedir_valarm_write_to_channel	(MIMEDirVAlarm *valarm, GIOChannel *channel, GError **error);
gchar		*mimedir_valarm_write_to_string		(MIMEDirVAlarm *valarm);

void		 mimedir_valarm_set_alarm_type		(MIMEDirVAlarm *valarm, MIMEDirVAlarmType type);
MIMEDirVAlarmType
		 mimedir_valarm_get_alarm_type		(MIMEDirVAlarm *valarm);

MIMEDirDateTime	*mimedir_valarm_get_trigger		(MIMEDirVAlarm *valarm, MIMEDirDateTime *start, MIMEDirDateTime *end);
MIMEDirDateTime	*mimedir_valarm_get_trigger_from_vcomponent
							(MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent);
MIMEDirDateTime	*mimedir_valarm_get_next_occurence	(MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent, MIMEDirDateTime *after);
gchar		*mimedir_valarm_get_reminder_string	(MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent);

#endif /* __MIMEDIR_VALARM_H__ */
