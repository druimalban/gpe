/* RFC 2445 iCal Recurrence Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 *
 * $Id: mimedir-recurrence.c 235 2005-10-08 06:46:36Z ituohela $
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

#include "mimedir-attribute.h"
#include "mimedir-datetime.h"
#include "mimedir-profile.h"
#include "mimedir-recurrence.h"
#include "mimedir-utils.h"

#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif

static void	 mimedir_recurrence_class_init		(MIMEDirRecurrenceClass	*klass);
static void	 mimedir_recurrence_init		(MIMEDirRecurrence	*recurrence);
static void	 mimedir_recurrence_dispose		(GObject		*object);

static void	 mimedir_recurrence_set_property	(GObject          *object,
			   				 guint             property_id,
							 const GValue     *value,
							 GParamSpec       *pspec);

static void	 mimedir_recurrence_get_property	(GObject          *object,
			   				 guint             property_id,
							 GValue     *value,
							 GParamSpec       *pspec);

enum {
	PROP_FREQ = 1,
	PROP_UNTIL,
	PROP_COUNT,
	PROP_INTERVAL,
	PROP_UNIT,
	PROP_UNITLIST
};

struct _MIMEDirRecurrencePriv {
	MIMEDirRecurrenceFrequency 	freq;
	MIMEDirDateTime 	*until;
	guint8 			count;
	guint8			interval;

	MIMEDirRecurrenceUnit	unit;
	gchar			*units;
};

static GObjectClass *parent_class = NULL;

/*
 * Utility Functions
 */

static const gchar *
freq_to_string (MIMEDirRecurrenceFrequency freq)
{
	switch (freq) {
	case RECURRENCE_SECONDLY:
		return "SECONDLY";
	case RECURRENCE_MINUTELY:
		return "MINUTELY";
	case RECURRENCE_HOURLY:
		return "HOURLY";
	case RECURRENCE_DAILY:
		return "DAILY";
	case RECURRENCE_WEEKLY:
		return "WEEKLY";
	case RECURRENCE_MONTHLY:
		return "MONTHLY";
	case RECURRENCE_YEARLY:
		return "YEARLY";
	default:
		g_return_val_if_reached (NULL);
	}
}

static MIMEDirRecurrenceFrequency
string_to_freq (const gchar *s)
{
	if (strcmp (s, "SECONDLY") == 0)
		return RECURRENCE_SECONDLY;
	else if (strcmp (s, "MINUTELY") == 0)
		return RECURRENCE_MINUTELY;
	else if (strcmp (s, "HOURLY") == 0)
		return RECURRENCE_HOURLY;
	else if (strcmp (s, "DAILY") == 0)
		return RECURRENCE_DAILY;
	else if (strcmp (s, "WEEKLY") == 0)
		return RECURRENCE_WEEKLY;
	else if (strcmp (s, "MONTHLY") == 0)
		return RECURRENCE_MONTHLY;
	else if (strcmp (s, "YEARLY") == 0)
		return RECURRENCE_YEARLY;
	else
		return -1;
}

static const gchar *
unit_to_string (MIMEDirRecurrenceUnit unit)
{
	switch (unit) {
	case RECURRENCE_UNIT_SECOND:
		return "BYSECOND";
	case RECURRENCE_UNIT_MINUTE:
		return "BYMINUTE";
	case RECURRENCE_UNIT_HOUR:
		return "BYHOUR";
	case RECURRENCE_UNIT_DAY:
		return "BYDAY";
	case RECURRENCE_UNIT_MONTHDAY:
		return "BYMONTHDAY";
	case RECURRENCE_UNIT_YEARDAY:
		return "BYYEARDAY";
	case RECURRENCE_UNIT_WEEKNO:
		return "BYWEEKNO";
	case RECURRENCE_UNIT_MONTH:
		return "BYMONTH";
	default:
		g_return_val_if_reached (NULL);
	}
}

static MIMEDirRecurrenceUnit
string_to_unit (const gchar *s)
{
	if (strcmp (s, "BYSECOND") == 0)
		return RECURRENCE_UNIT_SECOND;
	else if (strcmp (s, "BYMINUTE") == 0)
		return RECURRENCE_UNIT_MINUTE;
	else if (strcmp (s, "BYHOUR") == 0)
		return RECURRENCE_UNIT_HOUR;
	else if (strcmp (s, "BYDAY") == 0)
		return RECURRENCE_UNIT_DAY;
	else if (strcmp (s, "BYMONTHDAY") == 0)
		return RECURRENCE_UNIT_MONTHDAY;
	else if (strcmp (s, "BYYEARDAY") == 0)
		return RECURRENCE_UNIT_YEARDAY;
	else if (strcmp (s, "BYWEEKNO") == 0)
		return RECURRENCE_UNIT_WEEKNO;
	else if (strcmp (s, "BYMONTH") == 0)
		return RECURRENCE_UNIT_MONTH;
	else
		return -1;
}

/*
 * Class and Object Management
 */

GType
mimedir_recurrence_get_type (void)
{
	static GType mimedir_recurrence_type = 0;

	if (!mimedir_recurrence_type) {
		static const GTypeInfo mimedir_recurrence_info = {
			sizeof (MIMEDirRecurrenceClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_recurrence_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirRecurrence),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_recurrence_init,
		};

		mimedir_recurrence_type = g_type_register_static (G_TYPE_OBJECT,
								  "MIMEDirRecurrence",
								  &mimedir_recurrence_info,
								  0);
	}

	return mimedir_recurrence_type;
}

static void
mimedir_recurrence_class_init (MIMEDirRecurrenceClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;
	
	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_RECURRENCE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_recurrence_dispose;
        gobject_class->set_property = mimedir_recurrence_set_property;
        gobject_class->get_property = mimedir_recurrence_get_property;

	parent_class = g_type_class_peek_parent (klass);
	
	/* Properties */
	
	pspec = g_param_spec_uint ("frequency",
				   _("Frequency"),
				   _("Frequency type of this recurrency rule"),
				   RECURRENCE_SECONDLY,
				   RECURRENCE_YEARLY,
				   RECURRENCE_DAILY,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FREQ, pspec);
	pspec = g_param_spec_object ("until",
				     _("Date of last recurrence"),
				     _("The date of the last recurrence of an event"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_UNTIL, pspec);
	pspec = g_param_spec_uint ("count",
				     _("Number of recurrences"),
				     _("The number of recurrences of an event"),
				     1,
				     G_MAXINT,
				     1,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_COUNT, pspec);
	pspec = g_param_spec_uint ("interval",
				     _("Recurrence interval"),
				     _("The interval of recurrences of an event"),
				     1,
				     G_MAXINT,
				     1,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_INTERVAL, pspec);
	pspec = g_param_spec_uint ("unit",
				   _("Recurrence unit"),
				   _("By which unit a recurrence should be repeated"),
				   RECURRENCE_UNIT_NONE,
				   RECURRENCE_UNIT_MONTH,
				   RECURRENCE_UNIT_NONE,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_UNIT, pspec);
	pspec = g_param_spec_string ("units",
				     _("Recurrence units"),
				     _("List of units by which a recurrence should be repeated"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_UNITLIST, pspec);
}


static void
mimedir_recurrence_init (MIMEDirRecurrence *recurrence)
{
	g_return_if_fail (recurrence != NULL);
	g_return_if_fail (MIMEDIR_IS_RECURRENCE (recurrence));

	recurrence->priv = g_new0 (MIMEDirRecurrencePriv, 1);
	recurrence->priv->freq = RECURRENCE_DAILY;
	recurrence->priv->unit = RECURRENCE_UNIT_NONE;
}


static void
mimedir_recurrence_dispose (GObject *object)
{
	MIMEDirRecurrence *recurrence;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_RECURRENCE (object));

	recurrence = MIMEDIR_RECURRENCE (object);
	
	if (recurrence->priv) {
		g_free (recurrence->priv->units);		
		
		g_free (recurrence->priv);
	}

	recurrence->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_recurrence_set_property (GObject          *object,
				 guint             property_id,
				 const GValue     *value,
				 GParamSpec       *pspec)
{
        MIMEDirRecurrencePriv *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MIMEDIR_IS_RECURRENCE (object));

        priv = MIMEDIR_RECURRENCE (object)->priv;

	/* FIXME Check boundaries */

        switch (property_id) {
        case PROP_FREQ:
		priv->freq = g_value_get_uint (value);
		break;
	case PROP_UNTIL:
		if (priv->until != NULL)
			g_object_unref (G_OBJECT (priv->until));
		priv->until = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->until));
		break;
	case PROP_COUNT:
		priv->count = g_value_get_uint (value);
		break;
	case PROP_INTERVAL:
		priv->interval = g_value_get_uint (value);
		break;
        case PROP_UNIT:
		priv->unit = g_value_get_uint (value);
		break;
	case PROP_UNITLIST:
		mimedir_utils_set_property_string (&priv->units, value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
							
}

static void
mimedir_recurrence_get_property (GObject          *object,
				 guint             property_id,
				 GValue 	  *value,
				 GParamSpec       *pspec)
{
        MIMEDirRecurrencePriv *priv;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MIMEDIR_IS_RECURRENCE (object));

        priv = MIMEDIR_RECURRENCE (object)->priv;

	switch (property_id) {
	case PROP_FREQ:
		g_value_set_uint (value, priv->freq);
		break;
	case PROP_UNTIL:
		if (priv->until)
			g_value_set_object (value, G_OBJECT (priv->until));
		break;
	case PROP_COUNT:
		g_value_set_uint (value, priv->count);
		break;
	case PROP_INTERVAL:
		g_value_set_uint (value, priv->interval);
		break;
        case PROP_UNIT:
		g_value_set_uint (value, priv->unit);
		break;
	case PROP_UNITLIST:
		g_value_set_string (value, priv->units);
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
 * mimedir_recurrence_new:
 *
 * Creates a new iCal recurrence object.
 *
 * Return value: a new recurrence object
 **/
MIMEDirRecurrence *
mimedir_recurrence_new (void)
{
	MIMEDirRecurrence *recur;

	recur = g_object_new (MIMEDIR_TYPE_RECURRENCE, NULL);

	return recur;
}

/**
 * mimedir_recurrence_new_parse:
 *
 * Creates a new iCal recurrence object and fills it with data
 * from @attr. If there is a parse error, %NULL is returned and
 * @error is set appropriately.
 *
 * @attr: attribute, whose value is parsed
 * @error: error storage location
 *
 * Return value: a new recurrence object or %NULL
 **/
MIMEDirRecurrence *
mimedir_recurrence_new_parse (MIMEDirAttribute *attr, GError **error)
{
	MIMEDirRecurrence *recur;

	g_return_val_if_fail (attr != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attr), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	recur = g_object_new (MIMEDIR_TYPE_RECURRENCE, NULL);

	if (mimedir_recurrence_parse (recur, attr, error))
		return recur;

	g_object_unref (G_OBJECT (recur));

	return NULL;
}

/**
 * mimedir_recurrence_parse:
 *
 * Parse the supplied MIMEDirAttribute object's value into this
 * recurrence object.
 *
 * @recurrence: recurrence object
 * @attr: attribute, whose value is parsed
 * @error: error storage location
 *
 * Return value: success indicator
 **/
gboolean
mimedir_recurrence_parse (MIMEDirRecurrence *recurrence, MIMEDirAttribute *attr, GError **error)
{
	gboolean ret = TRUE;
	GError *err = NULL;
	GSList *list, *iter;

	g_return_val_if_fail (recurrence != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_RECURRENCE (recurrence), FALSE);
	g_return_val_if_fail (attr != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attr), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	list = mimedir_attribute_get_value_parameters (attr, &err);
	if (err) {
		g_propagate_error (error, err);
		return FALSE;
	}

	/* list is guaranteed to contain an even number of entries. */

	iter = list;
	while (iter != NULL) {
		gchar *name, *value;

		name = (gchar *) iter->data;
		iter = g_slist_next (iter);
		value = (gchar *) iter->data;
		iter = g_slist_next (iter);

		if (!strcasecmp (name, "FREQ")) {
			MIMEDirRecurrenceFrequency freq;

			freq = string_to_freq (value);
			if (freq == -1) {
				ret = FALSE;
				break;
			}
			g_object_set (G_OBJECT (recurrence), "frequency", freq, NULL);
		}
		else if (!strcasecmp (name, "UNTIL")) {
			MIMEDirDateTime *until;

			until = mimedir_datetime_new_parse (value);
			if (!until) {
				ret = FALSE;
				break;
			}

			g_object_set (G_OBJECT (recurrence), "until", until, NULL);
		}
		else if (!strcasecmp (name, "COUNT")) {
			g_object_set (G_OBJECT (recurrence), "count", atoi (value), NULL);
		}
		else if (!strcasecmp (name, "INTERVAL")) {
			g_object_set (G_OBJECT (recurrence), "interval", atoi (value), NULL);
		}
		else if (!strncasecmp (name, "BY", 2)) {
			MIMEDirRecurrenceUnit unit;
			unit = string_to_unit (name);
			if (unit == -1) {
				ret = FALSE;
				break;
			}
			g_object_set (G_OBJECT (recurrence), "unit", unit, NULL);
			g_object_set (G_OBJECT (recurrence), "units", value, NULL);
		}
		else {
			ret = FALSE;
			break;
		}
	}

	mimedir_attribute_free_string_list (list);

	if (!ret) {
		const gchar *attr_name = mimedir_attribute_get_name (attr);
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, attr_name);
	}

	return ret;
}

/**
 * mimedir_recurrence_write_to_string:
 *
 * Returns a string representation of @recurrence. This string is suitable
 * as RRULE value. Free the returned string with g_free().
 *
 * @recurrence: a #MIMEDirRecurrence object
 *
 * Return value: a string
 **/
gchar *
mimedir_recurrence_write_to_string (MIMEDirRecurrence *recurrence)
{
	MIMEDirRecurrencePriv *priv;

	g_return_val_if_fail (recurrence != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_RECURRENCE (recurrence), NULL);

	priv = recurrence->priv;

	/* FIXME Which are crucial? */
	
	GString *string = g_string_new ("");
	g_string_printf (string, "FREQ=%s", freq_to_string(priv->freq));

	if (priv->until) {
		GDateYear year;
		GDateMonth month;
		GDateDay day;
		mimedir_datetime_get_date (priv->until, &year, &month, &day);
		g_string_append_printf (string, ";UNTIL=%d%02d%02d", year, month, day);
	}

	if (priv->interval) {
		g_string_append_printf (string, ";INTERVAL=%d", priv->interval);
	}

	if (priv->unit != RECURRENCE_UNIT_NONE && priv->units) {
		const gchar *unit;
		unit = unit_to_string (priv->unit);
		g_string_append_printf (string, ";%s=%s", unit, priv->units);
	}

	return g_string_free (string, FALSE);
}
