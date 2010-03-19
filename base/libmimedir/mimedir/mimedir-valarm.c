/* RFC 2445 vAlarm MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-valarm.c 176 2005-02-26 22:46:04Z srittau $
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

#include "mimedir-utils.h"
#include "mimedir-valarm.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_valarm_class_init	(MIMEDirVAlarmClass	*klass);
static void	 mimedir_valarm_init		(MIMEDirVAlarm		*valarm);
static void	 mimedir_valarm_dispose		(GObject		*object);


struct _MIMEDirVAlarmPriv {
	MIMEDirVAlarmType type;
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_valarm_get_type (void)
{
	static GType mimedir_valarm_type = 0;

	if (!mimedir_valarm_type) {
		static const GTypeInfo mimedir_valarm_info = {
			sizeof (MIMEDirVAlarmClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_valarm_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVAlarm),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_valarm_init,
		};

		mimedir_valarm_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVAlarm",
							     &mimedir_valarm_info,
							     0);
	}

	return mimedir_valarm_type;
}


static void
mimedir_valarm_class_init (MIMEDirVAlarmClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VALARM_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_valarm_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_valarm_init (MIMEDirVAlarm *valarm)
{
	MIMEDirVAlarmPriv *priv;

	g_return_if_fail (valarm != NULL);
	g_return_if_fail (MIMEDIR_IS_VALARM (valarm));

	priv = g_new0 (MIMEDirVAlarmPriv, 1);
	valarm->priv = priv;
}


static void
mimedir_valarm_dispose (GObject *object)
{
	MIMEDirVAlarm *valarm;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VALARM (object));

	valarm = MIMEDIR_VALARM (object);

	g_free (valarm->priv);
	valarm->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_valarm_new:
 * @type: the alarm type
 *
 * Creates a new (empty) vAlarm object.
 *
 * Return value: a new vAlarm object
 **/
MIMEDirVAlarm *
mimedir_valarm_new (MIMEDirVAlarmType type)
{
	MIMEDirVAlarm *valarm;

	valarm = g_object_new (MIMEDIR_TYPE_VALARM, NULL);

	valarm->priv->type = type;

	return valarm;
}

/**
 * mimedir_valarm_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vAlarm object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vAlarm object or %NULL
 **/
MIMEDirVAlarm *
mimedir_valarm_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVAlarm *valarm;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	valarm = g_object_new (MIMEDIR_TYPE_VALARM, NULL);

	if (!mimedir_valarm_read_from_profile (valarm, profile, error)) {
		g_object_unref (G_OBJECT (valarm));
		valarm = NULL;
	}

	return valarm;
}

/**
 * mimedir_valarm_read_from_profile:
 * @valarm: a vAlarm object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vAlarm object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_valarm_read_from_profile (MIMEDirVAlarm *valarm, MIMEDirProfile *profile, GError **error)
{
	MIMEDirVAlarmPriv *priv;
	gchar *name;
	gchar *action;

	g_return_val_if_fail (valarm != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = valarm->priv;

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VALARM") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (valarm), profile, error))
		return FALSE;

	/* Validity checks */

	g_object_get (G_OBJECT (valarm),
		      "action", &action,
		      NULL);

	if (!action) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING_STR, "ACTION");
		return FALSE;
	}

	if (!g_ascii_strcasecmp (action, "AUDIO"))
		priv->type = MIMEDIR_VALARM_AUDIO;
	else if (!g_ascii_strcasecmp (action, "DISPLAY"))
		priv->type = MIMEDIR_VALARM_DISPLAY;
	else if (!g_ascii_strcasecmp (action, "EMAIL"))
		priv->type = MIMEDIR_VALARM_EMAIL;
	else if (!g_ascii_strcasecmp (action, "PROCEDURE"))
		priv->type = MIMEDIR_VALARM_PROCEDURE;
	else
		priv->type = MIMEDIR_VALARM_UNKNOWN;

	g_free (action);

	{
		guint duration, repeat;

		g_object_get (G_OBJECT (valarm),
			      "duration", &duration,
			      "repeat",   &repeat,
			      NULL);

		if (duration > 0  && repeat == 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING_STR, "REPEAT");
			return FALSE;
		}
		if (duration == 0  && repeat > 0) {
		  /* HACK - some buggy programs generate repeating alarms with a duration of 0,
		     even though the RFC says "positive" (not "non-negative").  We have no way
		     to distinguish a duration of 0 from a missing duration so the
		     following error is disabled and replaced with a printed warning */

		  /* g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING_STR, "DURATION");
		     return FALSE; */

		  g_printerr (_("Repeating alarm DURATION is either missing or 0 -- error ignored.\n"));
		}
	}

	return TRUE;
}

/**
 * mimedir_valarm_write_to_profile:
 * @valarm: a valarm
 *
 * Saves the valarm object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_valarm_write_to_profile (MIMEDirVAlarm *valarm)
{
	g_return_val_if_fail (valarm != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (valarm), "vAlarm");
}

/**
 * mimedir_valarm_write_to_channel:
 * @valarm: a valarm
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the valarm object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_valarm_write_to_channel (MIMEDirVAlarm *valarm, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (valarm != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_valarm_write_to_profile (valarm);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_valarm_write_to_string:
 * @valarm: a valarm
 *
 * Saves the valarm object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_valarm_write_to_string (MIMEDirVAlarm *valarm)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (valarm != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), FALSE);

	profile = mimedir_valarm_write_to_profile (valarm);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}

/**
 * mimedir_valarm_set_alarm_type:
 * @valarm: a #MIMEDirVAlarm object
 * @type: the new type
 *
 * Sets the notification type of the #MIMEDirVAlarm object.
 **/
void
mimedir_valarm_set_alarm_type (MIMEDirVAlarm *valarm, MIMEDirVAlarmType type)
{
	g_return_if_fail (valarm != NULL);
	g_return_if_fail (MIMEDIR_IS_VALARM (valarm));
	g_return_if_fail (type < MIMEDIR_VALARM_UNKNOWN || type > MIMEDIR_VALARM_PROCEDURE);

	valarm->priv->type = type;
}

/**
 * mimedir_valarm_get_alarm_type:
 * @valarm: a #MIMEDirVAlarm object
 *
 * Returns the notification type of the #MIMEDirVAlarm object.
 *
 * Return value: alarm type
 **/
MIMEDirVAlarmType
mimedir_valarm_get_alarm_type (MIMEDirVAlarm *valarm)
{
	g_return_val_if_fail (valarm != NULL, MIMEDIR_VALARM_UNKNOWN);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), MIMEDIR_VALARM_UNKNOWN);

	return valarm->priv->type;
}

/**
 * mimedir_valarm_get_trigger:
 * @valarm: a #MIMEDirVAlarm object
 * @start: a #MIMEDirDateTime object
 * @end: a #MIMEDirDateTime object or %NULL
 *
 * Returns a newly allocated #MIMEDirDateTime object that relates the
 * trigger time of the vAlarm object, depending on the supplied @start
 * or @end clamp times. If @end is %NULL it is assumed that @end equals
 * @start.
 *
 * The returned object should be freed with g_object_unref().
 *
 * Return value: a new #MIMEDirDateTime object
 **/

#define SECS_PER_DAY (60 * 60 * 24)

MIMEDirDateTime	*
mimedir_valarm_get_trigger (MIMEDirVAlarm *valarm, MIMEDirDateTime *start, MIMEDirDateTime *end)
{
	MIMEDirDateTime *dt;
	MIMEDirDateTime *trigger_dt;
	guint trigger;
	gboolean trigger_end;

	g_return_val_if_fail (valarm != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), NULL);
	g_return_val_if_fail (start != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (start), NULL);
	g_return_val_if_fail (end == NULL || MIMEDIR_IS_DATETIME (end), NULL);

	g_object_get (G_OBJECT (valarm),
		      "trigger",          &trigger,
		      "trigger-datetime", &trigger_dt,
		      "trigger-end",      &trigger_end,
		      NULL);

	if (trigger_end && end != NULL)
		start = end;

	if (trigger_dt && mimedir_datetime_is_valid (trigger_dt))
		dt = mimedir_datetime_new_from_datetime (start->year,
							 start->month,
							 start->day,
							 start->hour,
							 start->minute,
							 start->second);
	else {
		time_t t;

		t  = mimedir_datetime_get_time_t (start);
		t += trigger;
		dt = mimedir_datetime_new_from_time_t (t);
	}

	if (trigger_dt)
		g_object_unref (G_OBJECT (trigger_dt));

	return dt;
}

/**
 * mimedir_valarm_get_trigger_from_vcomponent:
 * @valarm: a #MIMEDirVAlarm object
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns a newly allocated #MIMEDirDateTime object that relates the
 * trigger time of the vAlarm object, corresponding to the start and
 * end date of @component.
 *
 * The returned object should be freed with g_object_unref().
 *
 * Return value: a new #MIMEDirDateTime object
 **/
MIMEDirDateTime	*
mimedir_valarm_get_trigger_from_vcomponent (MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent)
{
	MIMEDirDateTime *dt, *dtstart, *dtend;

	g_return_val_if_fail (valarm != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), NULL);
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	g_object_get (G_OBJECT (vcomponent),
		      "dtstart", &dtstart,
		      "dtend",   &dtend,
		      NULL);

	g_return_val_if_fail (dtstart != NULL, NULL);

	dt = mimedir_valarm_get_trigger (valarm, dtstart, dtend);

	g_object_unref (G_OBJECT (dtstart));
	if (dtend)
		g_object_unref (G_OBJECT (dtend));

	return dt;
}

/**
 * mimedir_valarm_get_next_occurence:
 * @valarm: a #MIMEDirVAlarm object
 * @vcomponent: a #MIMEDirVComponent object
 * @after: a #MIMEDirDateTime object
 *
 * Returns the first occurence of this alarm after or at the date specified by
 * the @after argument. If @after is %NULL, the very first occurence is
 * returned. If there is no occurence after the specified date, %NULL is
 * returned.
 *
 * Return value: a #MIMEDirDateTime object or %NULL
 **/
MIMEDirDateTime *
mimedir_valarm_get_next_occurence (MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent, MIMEDirDateTime *after)
{
	MIMEDirDateTime *trigger;

	g_return_val_if_fail (valarm != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), NULL);
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);
	g_return_val_if_fail (after != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (after), NULL);

	trigger = mimedir_valarm_get_trigger_from_vcomponent (valarm, vcomponent);
	if (!after)
		return trigger;

	if (mimedir_datetime_compare (trigger, after) >= 0)
		return trigger;

	/* FIXME */

	g_object_unref (G_OBJECT (trigger));

	return NULL;
}

/**
 * mimedir_valarm_get_reminder_string:
 * @valarm: a #MIMEDirVAlarm object
 * @vcomponent: the corresponding #MIMEDirVComponent object
 *
 * Return a string that can be presented to the user as reminder for
 * the corresponding appointment or to-do list entry. The returned
 * string should be freed with g_free().
 *
 * Return value: a reminder string
 **/
gchar *
mimedir_valarm_get_reminder_string (MIMEDirVAlarm *valarm, MIMEDirVComponent *vcomponent)
{
	MIMEDirDateTime *dtstart;
	gchar *desc, *date, *msg;

	g_return_val_if_fail (valarm != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VALARM (valarm), NULL);
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	g_object_get (G_OBJECT (valarm),
		      "description", &desc,
		      NULL);
	g_object_get (G_OBJECT (vcomponent),
		      "dtstart", &dtstart,
		      NULL);

	g_assert (desc != NULL);
	g_assert (mimedir_datetime_is_valid (dtstart));

	date = mimedir_datetime_to_string (dtstart);
	g_object_unref (G_OBJECT (dtstart));

	msg = g_strdup_printf (_("Reminder of your appointment:\n\n%s\n%s"),
			       date, desc);

	g_free (date);
	g_free (desc);

	return msg;
}
