/* MIME Directory Date/Time Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-datetime.h 214 2005-09-01 16:42:06Z srittau $
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

#ifndef __MIMEDIR_DATETIME_H__
#define __MIMEDIR_DATETIME_H__

#include <time.h>

#include <glib.h>
#include <glib-object.h>


#define MIMEDIR_TYPE_DATETIME			(mimedir_datetime_get_type())
#define MIMEDIR_DATETIME(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_DATETIME, MIMEDirDateTime))
#define MIMEDIR_DATETIME_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_DATETIME, MIMEDirDateTimeClass))
#define MIMEDIR_IS_DATETIME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_DATETIME))
#define MIMEDIR_IS_DATETIME_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_DATETIME))

typedef enum _MIMEDirDateTimeFlags MIMEDirDateTimeFlags;

enum _MIMEDirDateTimeFlags
{
	MIMEDIR_DATETIME_DATE = 1 << 0,
	MIMEDIR_DATETIME_TIME = 1 << 1
};

#define MIMEDIR_DATETIME_NOTZ (30000)
#define MIMEDIR_DATETIME_UTC  (30001)


typedef struct _MIMEDirDateTime		MIMEDirDateTime;
typedef struct _MIMEDirDateTimeClass	MIMEDirDateTimeClass;
typedef struct _MIMEDirDateTimePriv	MIMEDirDateTimePriv;

struct _MIMEDirDateTime
{
	GObject parent;

	GDateYear  year;
	GDateMonth month;
	GDateDay   day;
	guint8     hour;
	guint8     minute;
	guint8     second;
	gint16     timezone;

	MIMEDirDateTimeFlags flags;

	MIMEDirDateTimePriv *priv;
};

struct _MIMEDirDateTimeClass
{
	GObjectClass parent_class;
};


GType		 mimedir_datetime_get_type		(void);

MIMEDirDateTime	*mimedir_datetime_new			(void);
MIMEDirDateTime	*mimedir_datetime_new_now		(void);
MIMEDirDateTime *mimedir_datetime_new_from_gdate	(const GDate *date);
MIMEDirDateTime	*mimedir_datetime_new_from_struct_tm	(const struct tm *tm);
MIMEDirDateTime	*mimedir_datetime_new_from_time_t	(time_t t);
MIMEDirDateTime *mimedir_datetime_new_from_datetime	(GDateYear year, GDateMonth month, GDateDay day, guint8 hour, guint8 minute, guint8 second);
MIMEDirDateTime *mimedir_datetime_new_from_date		(GDateYear year, GDateMonth month, GDateDay day);
MIMEDirDateTime *mimedir_datetime_new_from_time		(guint8 hour, guint8 minute, guint8 second);
MIMEDirDateTime *mimedir_datetime_new_parse		(const gchar *str);

void		 mimedir_datetime_set_gdate		(MIMEDirDateTime *dt, const GDate *date);
void		 mimedir_datetime_get_gdate		(MIMEDirDateTime *dt, GDate *date);
void		 mimedir_datetime_set_struct_tm		(MIMEDirDateTime *dt, const struct tm *tm);
void		 mimedir_datetime_get_struct_tm		(MIMEDirDateTime *dt, struct tm *tm);
void		 mimedir_datetime_set_time_t		(MIMEDirDateTime *dt, time_t t);
time_t		 mimedir_datetime_get_time_t		(MIMEDirDateTime *dt);
void		 mimedir_datetime_set_datetime		(MIMEDirDateTime *dt, GDateYear year, GDateMonth month, GDateDay day, guint8 hour, guint8 minute, guint8 second);
void		 mimedir_datetime_get_datetime		(MIMEDirDateTime *dt, GDateYear *year, GDateMonth *month, GDateDay *day, guint8 *hour, guint8 *minute, guint8 *second);
void		 mimedir_datetime_set_date		(MIMEDirDateTime *dt, GDateYear year, GDateMonth month, GDateDay day);
void		 mimedir_datetime_get_date		(MIMEDirDateTime *dt, GDateYear *year, GDateMonth *month, GDateDay *day);
void		 mimedir_datetime_set_time		(MIMEDirDateTime *dt, guint8 hour, guint8 minute, guint8 second);
void		 mimedir_datetime_get_time		(MIMEDirDateTime *dt, guint8 *hour, guint8 *minute, guint8 *second);

gboolean	 mimedir_datetime_is_valid		(MIMEDirDateTime *dt);
gboolean	 mimedir_datetime_is_valid_date		(GDateYear year, GDateMonth month, GDateDay day);
gboolean	 mimedir_datetime_is_valid_time		(guint8 hour, guint8 minute, guint8 second);

gboolean	 mimedir_datetime_parse			(MIMEDirDateTime *dt, const gchar *str);

void		 mimedir_datetime_to_utc		(MIMEDirDateTime *dt);

gint		 mimedir_datetime_compare		(MIMEDirDateTime *dt1, MIMEDirDateTime *dt2);

gchar		*mimedir_datetime_to_string		(MIMEDirDateTime *dt);

#endif /* __MIMEDIR_DATETIME_H__ */
