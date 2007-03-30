/* MIME Directory Date/Time Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-datetime.c 214 2005-09-01 16:42:06Z srittau $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libintl.h>

#include "mimedir-datetime.h"
#include "mimedir-utils.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_datetime_class_init	(MIMEDirDateTimeClass	*klass);
static void	 mimedir_datetime_init		(MIMEDirDateTime	*datetime);
static void	 mimedir_datetime_dispose	(GObject		*object);
static void	 mimedir_datetime_set_property	(GObject		*object,
						 guint			 property_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 mimedir_datetime_get_property	(GObject		*object,
						 guint			 property_id,
						 GValue			*value,
						 GParamSpec		*pspec);


enum {
	PROP_TZID = 1
};


struct _MIMEDirDateTimePriv {
	gchar *tzid;
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_datetime_get_type (void)
{
	static GType mimedir_datetime_type = 0;

	if (!mimedir_datetime_type) {
		static const GTypeInfo mimedir_datetime_info = {
			sizeof (MIMEDirDateTimeClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_datetime_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirDateTime),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_datetime_init,
		};

		mimedir_datetime_type = g_type_register_static (G_TYPE_OBJECT,
								  "MIMEDirDateTime",
								  &mimedir_datetime_info,
								  0);
	}

	return mimedir_datetime_type;
}


static void
mimedir_datetime_class_init (MIMEDirDateTimeClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_datetime_dispose;
	gobject_class->set_property = mimedir_datetime_set_property;
	gobject_class->get_property = mimedir_datetime_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Properties */

	pspec = g_param_spec_string ("tzid",
				     _("Time zone identifier"),
				     _("Time zone identifying string"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZID, pspec);
}


static void
mimedir_datetime_init (MIMEDirDateTime *datetime)
{
	MIMEDirDateTimePriv *priv;

	g_return_if_fail (datetime != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (datetime));

	priv = g_new0 (MIMEDirDateTimePriv, 1);
	datetime->priv = priv;
}


static void
mimedir_datetime_dispose (GObject *object)
{
	MIMEDirDateTime *datetime;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (object));

	datetime = MIMEDIR_DATETIME (object);

	if (datetime->priv) {
		g_free (datetime->priv->tzid);
		g_free (datetime->priv);
		datetime->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
mimedir_datetime_set_property (GObject		*object,
			       guint		 property_id,
			       const GValue	*value,
			       GParamSpec	*pspec)
{
	MIMEDirDateTime *dt;
	MIMEDirDateTimePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (object));

	dt = MIMEDIR_DATETIME (object);
	priv = dt->priv;

	switch (property_id) {
	case PROP_TZID:
		mimedir_utils_set_property_string (&priv->tzid, value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
mimedir_datetime_get_property (GObject		*object,
			       guint		 property_id,
			       GValue		*value,
			       GParamSpec	*pspec)
{
	MIMEDirDateTime *dt;
	MIMEDirDateTimePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (object));

	dt = MIMEDIR_DATETIME (object);
	priv = dt->priv;

	switch (property_id) {
	case PROP_TZID:
		g_value_set_string (value, priv->tzid);
		break;
 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Public Methods
 */

/**
 * mimedir_datetime_new:
 *
 * Creates a new MIME Directory Date/Time object.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new (void)
{
	return g_object_new (MIMEDIR_TYPE_DATETIME, NULL);
}

/**
 * mimedir_datetime_new_now:
 *
 * Creates a new MIME Directory Date/Time object. The date and time will
 * be set to the current date/time.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_now (void)
{
	return mimedir_datetime_new_from_time_t (time (NULL));
}

/**
 * mimedir_datetime_new_from_gdate:
 * @date: a #GDate object
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * date from @date.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_gdate (const GDate *date)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (date != NULL, NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_gdate (dt, date);

	return dt;
}

/**
 * mimedir_datetime_new_from_struct_tm:
 * @tm: a struct tm pointer
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * date from @date.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_struct_tm (const struct tm *tm)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (tm != NULL, NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_struct_tm (dt, tm);

	return dt;
}

/**
 * mimedir_datetime_new_from_time_t:
 * @t: time
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * date from @t.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_time_t (time_t t)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (t >= 0, NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_time_t (dt, t);

	return dt;
}

/**
 * mimedir_datetime_new_from_datetime:
 * @year:
 * @month:
 * @day:
 * @hour:
 * @minute:
 * @second:
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * supplied date and time.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_datetime (GDateYear year, GDateMonth month, GDateDay day, guint8 hour, guint8 minute, guint8 second)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (mimedir_datetime_is_valid_date (year, month, day), NULL);
	g_return_val_if_fail (mimedir_datetime_is_valid_time (hour, minute, second), NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_datetime (dt, year, month, day, hour, minute, second);

	return dt;
}

/**
 * mimedir_datetime_new_from_date:
 * @year:
 * @month:
 * @day:
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * supplied date.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_date (GDateYear year, GDateMonth month, GDateDay day)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (mimedir_datetime_is_valid_date (year, month, day), NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_date (dt, year, month, day);

	return dt;
}

/**
 * mimedir_datetime_new_from_time:
 * @hour:
 * @minute:
 * @second:
 *
 * Creates a new MIME Directory Date/Time object and fills it with the
 * supplied time.
 *
 * Return value: a new date/time object
 **/
MIMEDirDateTime *
mimedir_datetime_new_from_time (guint8 hour, guint8 minute, guint8 second)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (mimedir_datetime_is_valid_time (hour, minute, second), NULL);

	dt = mimedir_datetime_new ();

	mimedir_datetime_set_time (dt, hour, minute, second);

	return dt;
}

/**
 * mimedir_datetime_new_parse:
 *
 * Creates a new MIME Directory Date/Time object by parsing @str.
 * If @str can't be parsed as a valid date/time, %NULL is returned
 *
 * @str: string to parse
 *
 * Return value: a new date/time object or %NULL
 **/
MIMEDirDateTime *
mimedir_datetime_new_parse (const gchar *str)
{
	MIMEDirDateTime *dt;

	g_return_val_if_fail (str != NULL, NULL);

	dt = mimedir_datetime_new ();
	if (mimedir_datetime_parse (dt, str))
		return dt;

	g_object_unref (G_OBJECT (dt));

	return NULL;
}

/**
 * mimedir_datetime_set_gdate:
 * @dt: a #MIMEDirDateTime object
 * @date: a #GDate object
 *
 * Sets @dt to the date of @date.
 **/
void
mimedir_datetime_set_gdate (MIMEDirDateTime *dt, const GDate *date)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (date != NULL);

	dt->year     = date->year;
	dt->month    = date->month;
	dt->day      = date->day;

	if (g_date_valid (date))
		dt->flags |= MIMEDIR_DATETIME_DATE;
	else
		dt->flags &= ~MIMEDIR_DATETIME_DATE;
}

/**
 * mimedir_datetime_get_gdate:
 * @dt: a #MIMEDirDateTime object
 * @date: a #GDate object
 *
 * Gets the date as a #GDate.
 **/
void
mimedir_datetime_get_gdate (MIMEDirDateTime *dt, GDate *date)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (date != NULL);

	if ((dt->flags & MIMEDIR_DATETIME_DATE) == 0) {
		g_critical ("MIMEDirDateTime object does not contain a valid date\n");
		return;
	}

	g_date_set_dmy (date, dt->day, dt->month, dt->year);
}

/**
 * mimedir_datetime_set_struct_tm:
 * @dt: a #MIMEDirDateTime object
 * @tm: a struct tm pointer
 *
 * Sets @dt to the date of @tm.
 **/
void
mimedir_datetime_set_struct_tm (MIMEDirDateTime *dt, const struct tm *tm)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (tm != NULL);

	dt->year     = tm->tm_year + 1900;
	dt->month    = tm->tm_mon + 1;
	dt->day      = tm->tm_mday;
	dt->hour     = tm->tm_hour;
	dt->minute   = tm->tm_min;
	dt->second   = tm->tm_sec;
	dt->timezone = MIMEDIR_DATETIME_UTC;

	dt->flags |= MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME;
}

/**
 * mimedir_datetime_get_struct_tm:
 * @dt: a #MIMEDirDateTime object
 * @tm: a struct tm pointer
 *
 * Gets the date as a #struct tm.
 **/
void
mimedir_datetime_get_struct_tm (MIMEDirDateTime *dt, struct tm *tm)
{
	time_t t;

	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (tm != NULL);

	memset (tm, 0, sizeof (struct tm));

	tm->tm_year = dt->year - 1900;
	tm->tm_mon  = dt->month - 1;
	tm->tm_mday = dt->day;
	tm->tm_hour = dt->hour;
	tm->tm_min  = dt->minute;
	tm->tm_sec  = dt->second;

	t = mktime (tm);
	localtime_r (&t, tm);
}
/**
 * mimedir_datetime_set_time_t:
 * @dt: a #MIMEDirDateTime object
 * @t: the time
 *
 * Sets @dt to the date of @t.
 **/
void
mimedir_datetime_set_time_t (MIMEDirDateTime *dt, time_t t)
{
	struct tm tm;

	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (t >= 0);

	gmtime_r (&t, &tm);

	mimedir_datetime_set_struct_tm (dt, &tm);
}

/**
 * mimedir_datetime_get_time_t:
 * @dt: a #MIMEDirDateTime object
 *
 * Gets the the date a #time_t. If the date is not representable as #time_t
 * -1 is returned.
 *
 * Return value: the time in seconds since the epoch or -1
 **/
time_t
mimedir_datetime_get_time_t (MIMEDirDateTime *dt)
{
	struct tm tm;

	g_return_val_if_fail (dt != NULL, -1);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt), -1);

	mimedir_datetime_get_struct_tm (dt, &tm);
	return mktime (&tm);
}

/**
 * mimedir_datetime_set_datetime:
 * @dt: a #MIMEDirDateTime object
 * @year:
 * @month:
 * @day:
 * @hour:
 * @minute:
 * @second:
 *
 * Sets the date and time of @dt.
 **/
void
mimedir_datetime_set_datetime (MIMEDirDateTime *dt, GDateYear year, GDateMonth month, GDateDay day, guint8 hour, guint8 minute, guint8 second)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (mimedir_datetime_is_valid_date (year, month, day));
	g_return_if_fail (mimedir_datetime_is_valid_time (hour, minute, second));

	dt->year   = year;
	dt->month  = month;
	dt->day    = day;
	dt->hour   = hour;
	dt->minute = minute;
	dt->second = second;

	dt->flags |= MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME;
}

/**
 * mimedir_datetime_get_datetime:
 * @dt: a #MIMEDirDateTime object
 * @year:
 * @month:
 * @day:
 * @hour:
 * @minute:
 * @second:
 *
 * Gets the date and time of @dt.
 **/
void
mimedir_datetime_get_datetime (MIMEDirDateTime *dt, GDateYear *year, GDateMonth *month, GDateDay *day, guint8 *hour, guint8 *minute, guint8 *second)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (dt->flags & MIMEDIR_DATETIME_DATE);
	g_return_if_fail (dt->flags & MIMEDIR_DATETIME_TIME);

	*year   = dt->year;
	*month  = dt->month;
	*day    = dt->day;
	*hour   = dt->hour;
	*minute = dt->minute;
	*second = dt->second;
}

/**
 * mimedir_datetime_set_date:
 * @dt: a #MIMEDirDateTime object
 * @year:
 * @month:
 * @day:
 *
 * Sets the date of @dt.
 **/
void
mimedir_datetime_set_date (MIMEDirDateTime *dt, GDateYear year, GDateMonth month, GDateDay day)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (mimedir_datetime_is_valid_date (year, month, day));

	dt->year   = year;
	dt->month  = month;
	dt->day    = day;

	dt->flags |= MIMEDIR_DATETIME_DATE;
}

/**
 * mimedir_datetime_get_date:
 * @dt: a #MIMEDirDateTime object
 * @year:
 * @month:
 * @day:
 *
 * Gets the date of @dt.
 **/
void
mimedir_datetime_get_date (MIMEDirDateTime *dt, GDateYear *year, GDateMonth *month, GDateDay *day)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (dt->flags & MIMEDIR_DATETIME_DATE);

	*year   = dt->year;
	*month  = dt->month;
	*day    = dt->day;
}

/**
 * mimedir_datetime_set_time:
 * @dt: a #MIMEDirDateTime object
 * @hour:
 * @minute:
 * @second:
 *
 * Sets the time of @dt.
 **/
void
mimedir_datetime_set_time (MIMEDirDateTime *dt, guint8 hour, guint8 minute, guint8 second)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (mimedir_datetime_is_valid_time (hour, minute, second));

	dt->hour   = hour;
	dt->minute = minute;
	dt->second = second;

	dt->flags |= MIMEDIR_DATETIME_TIME;
}

/**
 * mimedir_datetime_get_time:
 * @dt: a #MIMEDirDateTime object
 * @hour:
 * @minute:
 * @second:
 *
 * Gets the time of @dt.
 **/
void
mimedir_datetime_get_time (MIMEDirDateTime *dt, guint8 *hour, guint8 *minute, guint8 *second)
{
	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));
	g_return_if_fail (dt->flags & MIMEDIR_DATETIME_TIME);

	*hour   = dt->hour;
	*minute = dt->minute;
	*second = dt->second;
}

/**
 * mimedir_datetime_is_valid:
 * @dt: a #MIMEDirDateTime object
 *
 * Checks whether @dt is has a valid date and/or time.
 *
 * Return value: validity indicator
 **/
gboolean
mimedir_datetime_is_valid (MIMEDirDateTime *dt)
{
	g_return_val_if_fail (dt != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt), FALSE);

	return dt->flags & (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME);
}

/**
 * mimedir_datetime_is_valid_date:
 * @year:
 * @month:
 * @day:
 *
 * Check whether the given date is valid.
 *
 * Return value: validity indicator
 **/
gboolean
mimedir_datetime_is_valid_date (GDateYear year, GDateMonth month, GDateDay day)
{
	if (year < 1 || year > 9999)
		return FALSE;

	return g_date_valid_dmy (day, month, year);
}

/**
 * mimedir_datetime_is_valid_time
 * @hour:
 * @minute:
 * @second:
 *
 * Check whether the given time is valid.
 *
 * Return value: validity indicator
 **/
gboolean
mimedir_datetime_is_valid_time (guint8 hour, guint8 minute, guint8 second)
{
	if (hour > 23)
		return FALSE;
	if (minute > 59)
		return FALSE;
	if (second > 59)
		return FALSE;
	return TRUE;
}

/**
 * mimedir_datetime_parse:
 * @dt: a #MIMEDirDateTime object
 * @str: string to parse
 *
 * Parses @str into this date/time object. Return %FALSE if the
 * given string couldn't be parsed as a valid date/time.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_datetime_parse (MIMEDirDateTime *dt, const gchar *str)
{
	gint i, type;
	guint year, month, day, hour, minute, second;
	gint16 tz = MIMEDIR_DATETIME_NOTZ;

	g_return_val_if_fail (dt != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt), FALSE);
	g_return_val_if_fail (str != NULL, FALSE);

	/* Dates/times have at least six digits/letters */

	for (i = 1; i < 6; i++) {
		if (str[i] == '\0')
			return FALSE;
	}

	/* Check whether the string starts with a date or a time */

	if (str[2] == ':' || str[4] == ':') { /* time */
		type = 1;
	} else if (str[4] == '-' || str[6] == '-') { /* date */
		type = 0;
	} else {
		/* It's not that easy. Now count the number of digits. */

		for (i = 0; str[i] >= '0' && str[i] <= '9'; i++)
			;
		if (i == 8)
			type = 0;
		else if (i == 6)
			type = 1;
		else
			return FALSE;
	}

	/* Read date */

	if (type == 0) {
		if (str[0] < '0' || str[0] > '9' ||
		    str[1] < '0' || str[1] > '9' ||
		    str[2] < '0' || str[1] > '9' ||
		    str[3] < '0' || str[3] > '9')
			return FALSE;

		year = (str[0] - '0') * 1000 +
		       (str[1] - '0') * 100 +
		       (str[2] - '0') * 10 +
		       (str[3] - '0');
		str += 4;

		if (str[0] == '-')
			str++;

		if (str[0] < '0' || str[0] > '1' ||
		    str[1] < '0' || str[1] > '9')
			return FALSE;

		month = (str[0] - '0') * 10 + (str[1] - '0');
		str += 2;

		if (str[0] == '-')
			str++;

		if (str[0] < '0' || str[0] > '3' ||
		    str[1] < '0' || str[1] > '9')
			return FALSE;

		day = (str[0] - '0') * 10 + (str[1] - '0');
		str += 2;

		if (str[0] == 'T' || str[0] == 't') {
			type = 2;
			str++;
		}
	}

	/* Read time */

	if (type > 0) {
		gboolean minus = FALSE;

		if (str[0] < '0' || str[0] > '2' ||
		    str[1] < '0' || str[1] > '9')
			return FALSE;

		hour = (str[0] - '0') * 10 + (str[1] - '0');
		str += 2;

		if (str[0] == ':')
			str++;

		if (str[0] < '0' || str[0] > '5' ||
		    str[1] < '0' || str[1] > '9')
			return FALSE;

		minute = (str[0] - '0') * 10 + (str[1] - '0');
		str += 2;

		if (str[0] == ':')
			str++;

		if (str[0] < '0' || str[0] > '5' ||
		    str[1] < '0' || str[1] > '9')
			return FALSE;

		second = (str[0] - '0') * 10 + (str[1] - '0');
		str += 2;

		/* Fraction */

		/* RFC2425 says in the ABNF definition on page 33
		 * that this must be comma. This is illogical, since
		 * it would create a non-unique grammar.
		 */
		if (str[0] == '.') {
			str++;
			for (; str[0] >= '0' && str[0] <= '9'; str++)
				;
		}

		/* Time zone */

		if (str[0] == 'Z') {
			str++;
			tz = MIMEDIR_DATETIME_UTC;
		} else if (str[0] == '+' || str[0] == '-') {
			if (str[0] == '-')
				minus = TRUE;

			if (str[0] < '0' || str[0] > '9' ||
			    str[1] < '0' || str[1] > '9')
				return FALSE;

			tz = (str[0] - '0') * 10 + (str[1] - '0');
			tz *= 60;
			str += 2;

			if (str[0] == ':')
				str++;

			if (str[0] < '0' || str[0] > '9' ||
			    str[1] < '0' || str[1] > '9')
				return FALSE;

			tz += (str[0] - '0') * 10 + (str[1] - '0');
		}
	}

	if (str[0] != '\0')
		return FALSE;

	if (type == 0 || type == 2) {
		if (!mimedir_datetime_is_valid_date (year, month, day))
			return FALSE;
	}
	if (type == 1 || type == 2) {
		if (!mimedir_datetime_is_valid_time (hour, month, day))
			return FALSE;
	}

	dt->flags = 0;

	if (type == 0 || type == 2)
		mimedir_datetime_set_date (dt, year, month, day);
	if (type == 1 || type == 2) {
		mimedir_datetime_set_time (dt, hour, minute, second);
		dt->timezone = tz;
	}

	return TRUE;
}

/**
 * mimedir_datetime_to_utc:
 * @dt: a #MIMEDirDateTime object
 *
 * Converts the date/time object to a UTC date/time.
 **/
void
mimedir_datetime_to_utc (MIMEDirDateTime *dt)
{
	gint hour, minute;

	g_return_if_fail (dt != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (dt));

	if (dt->timezone == MIMEDIR_DATETIME_UTC)
		return;
	if (dt->timezone == MIMEDIR_DATETIME_NOTZ) {
		dt->timezone = MIMEDIR_DATETIME_UTC;
		return;
	}
	if (!(dt->flags & MIMEDIR_DATETIME_TIME)) {
		dt->timezone = MIMEDIR_DATETIME_UTC;
		return;
	}

	g_assert (dt->timezone >= -23 * 60 && dt->timezone <= 23 * 60);

	hour = dt->hour;
	minute = dt->minute + dt->timezone;
	if (minute >= 60) {
		hour += minute / 60;
		minute %= 60;
	} else if (minute < 0) {
		hour += minute / 60 - 1;
		minute %= 60;
		minute = 60 + minute;
	}
	dt->minute = (guint) minute;

	if (hour > 23) {
		hour -= 24;
		dt->day++;
	} else if (hour < 0) {
		hour = 24 + hour;
		dt->day--;
	}
	dt->hour = (guint) hour;

	if (dt->day > g_date_get_days_in_month (dt->month, dt->year)) {
		dt->month++;
		if (dt->month > 12) {
			dt->month = 1;
			dt->year++;
		}
		dt->day = 1;
	} else if (dt->day < 1) {
		dt->month--;
		if (dt->month < 1) {
			dt->month = 12;
			dt->year--;
		}
		dt->day = g_date_get_days_in_month (dt->month, dt->year);
	}

	dt->timezone = MIMEDIR_DATETIME_UTC;
}

/**
 * mimedir_datetime_compare:
 * @dt1: a #MIMEDirDateTime object
 * @dt2: a second #MIMEDirDateTime object
 *
 * Compares the dates of @dt1 and @dt2 and return -1 if @dt1 is earlier than
 * @dt2, 0 if both dates are equal and 1 if @dt1 is later than @d2. Both
 * dates must be valid.
 *
 * Return value: -1, 0, or 1
 **/
gint
mimedir_datetime_compare (MIMEDirDateTime *dt1, MIMEDirDateTime *dt2)
{
	g_return_val_if_fail (dt1 != NULL, 0);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt1), 0);
	g_return_val_if_fail (dt2 != NULL, 0);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt2), 0);
	g_return_val_if_fail (mimedir_datetime_is_valid (dt1), 0);
	g_return_val_if_fail (mimedir_datetime_is_valid (dt2), 0);

	if ((dt1->flags & MIMEDIR_DATETIME_DATE) &&
	    (dt2->flags & MIMEDIR_DATETIME_DATE)) {
		if (dt1->year < dt2->year)
			return -1;
		else if (dt1->year > dt2->year)
			return 1;
		if (dt1->month < dt2->month)
			return -1;
		else if (dt1->month > dt2->month)
			return 1;
		if (dt1->day < dt2->day)
			return -1;
		else if (dt1->day > dt2->day)
			return 1;
	}

	if ((dt1->flags & MIMEDIR_DATETIME_TIME) &&
	    (dt2->flags & MIMEDIR_DATETIME_TIME)) {
		if (dt1->hour < dt2->hour)
			return -1;
		else if (dt1->hour > dt2->hour)
			return 1;
		if (dt1->minute < dt2->minute)
			return -1;
		else if (dt1->minute > dt2->minute)
			return 1;
		if (dt1->second < dt2->second)
			return -1;
		else if (dt1->second > dt2->second)
			return 1;
	}

	return 0;
}

/**
 * mimedir_datetime_to_string:
 * @dt: a #MIMEDirDateTime object
 *
 * Returns a human-readable, localized string that represents the date
 * and/or the time this object represent. In case that neither date nor
 * time is valid, NULL is returned. Otherwise the returned string must
 * be freed with g_free().
 *
 * Return value: human-readable string representing the date and/or time
 **/
gchar *
mimedir_datetime_to_string (MIMEDirDateTime *dt)
{
	g_return_val_if_fail (dt != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (dt), NULL);
	g_return_val_if_fail (mimedir_datetime_is_valid (dt), NULL);

	if ((dt->flags & (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME)) == (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME))
		/* Translators: YYYY-MM-DD HH:MM:SS */
		return g_strdup_printf (_("%04d-%02d-%02d %02d:%02d:%02d"),
					dt->year, dt->month, dt->day,
					dt->hour, dt->minute, dt->second);
	else if (dt->flags & MIMEDIR_DATETIME_DATE)
		/* Translators: YYYY-MM-DD */
		return g_strdup_printf (_("%04d-%02d-%02d"),
					dt->year, dt->month, dt->day);
	else if (dt->flags & MIMEDIR_DATETIME_TIME)
		/* Translators: HH:MM:SS */
		return g_strdup_printf (_("%02d:%02d:%02d"),
					dt->hour, dt->minute, dt->second);

	return NULL;
}
