/* RFC 2445 iCal Component Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 * Copyright (C) 2007  Graham Cobb <g+770@cobb.uk.net>
 *
 * $Id: mimedir-vcomponent.c 247 2005-11-26 15:40:20Z srittau $
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

#include <glib.h>

#include "mimedir-attachment.h"
#include "mimedir-attendee.h"
#include "mimedir-attribute.h"
#include "mimedir-datetime.h"
#include "mimedir-marshal.h"
#include "mimedir-period.h"
#include "mimedir-recurrence.h"
#include "mimedir-utils.h"
#include "mimedir-valarm.h"
#include "mimedir-vcomponent.h"
#include "mimedir-vfreebusy.h"
#include "mimedir-vtimezone.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcomponent_class_init		(MIMEDirVComponentClass	*klass);
static void	 mimedir_vcomponent_init		(MIMEDirVComponent	*vcomponent);
static void	 mimedir_vcomponent_dispose		(GObject		*object);
static void	 mimedir_vcomponent_set_property	(GObject		*object,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	 mimedir_vcomponent_get_property	(GObject		*object,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);

static void	 mimedir_vcomponent_clear		(MIMEDirVComponent	*vcomponent);
static void	 mimedir_vcomponent_changed		(MIMEDirVComponent	*vcomponent);

static void	 mimedir_vcomponent_set_value_duration	(MIMEDirAttribute	*attr, gint duration);
static gint	 mimedir_vcomponent_get_value_duration	(MIMEDirAttribute	*attr, GError **error);
static void	 mimedir_vcomponent_set_value_utcoffset	(MIMEDirAttribute	*attr, gint offset);
static gint	 mimedir_vcomponent_get_value_utcoffset	(MIMEDirAttribute	*attr, GError **error);


enum {
	PROP_CATEGORY_LIST = 1,
	PROP_COMMENT_LIST,
	PROP_DESCRIPTION,
	PROP_PERCENT_COMPLETE,
	PROP_PRIORITY,
	PROP_RESOURCE_LIST,
	PROP_STATUS,
	PROP_SUMMARY,

	PROP_DTCOMPLETED,
	PROP_DTEND,
	PROP_DUE,
	PROP_DTSTART,
	PROP_DURATION,
	PROP_FREEBUSY,

	PROP_TZID,
	PROP_TZNAME_LIST,
	PROP_TZOFFFROM,
	PROP_TZOFFTO,
	PROP_TZURL,

	PROP_CONTACT,

	PROP_UID,

	PROP_RECURRENCE,

	PROP_ACTION,
	PROP_REPEAT,
	PROP_TRIGGER,
	PROP_TRIGGER_DT,
	PROP_TRIGGER_END,

	PROP_CREATED,
	PROP_DTSTAMP,
	PROP_LAST_MODIFIED,
	PROP_SEQ
};

enum {
	SIGNAL_CHANGED,
	SIGNAL_ALARMS_CHANGED,
	SIGNAL_LAST
};

struct _MIMEDirVComponentPriv {
	gboolean	 changed;

	GList		*alarms;

	/* Descriptive Properties (RFC 2445, Section 4.8.1) */

	GList		*attachments;	/* Attach (4.8.1.1) */
	GList		*categories;	/* Categories (4.8.1.2) */
	MIMEDirClassification
			 md_class;	/* Class (4.8.1.3) */
	gchar		*md_class_str;
	GList		*comment;	/* Comment (4.8.1.4) */
	gchar		*description;	/* Description (4.8.1.5) */
	gdouble		 latitude, longitude;
					/* Geographic Position (4.8.1.6) */
	gchar		*location;	/* Location (4.8.1.7) */
	gchar		*location_alt;
	gint		 percent;	/* Percent Complete (4.8.1.8) */
	guint		 priority;	/* Priority (4.8.1.9) */
	GList		*resources;	/* Resources (4.8.1.10) */
	MIMEDirStatus	 status;	/* Status (4.8.1.11) */
	gchar		*summary;	/* Summary (4.8.1.12) */

	/* Date and Time Properties (RFC 2445, Section 4.8.2) */

	MIMEDirDateTime	*dtcompleted;	/* Date/Time Completed (4.8.2.1) */
	MIMEDirDateTime	*dtend;		/* Date/Time End (4.8.2.2) */
	MIMEDirDateTime	*due;		/* Date/Time Due (4.8.2.3) */
	MIMEDirDateTime	*dtstart;	/* Date/Time Start (4.8.2.4) */
	gint		 duration;	/* Duration (4.8.2.5) */
	GList		*freebusy;	/* Free/Busy Time (4.8.2.6) */
	gboolean	 opaque;	/* Time Transparency (4.8.2.7) */

	/* Time Zone Properties (RFC 2445, Section 4.8.3) */

	gchar		*tzid;		/* Time Zone Identifier (4.8.3.1) */
	GList		*tznames;	/* Time Zone Name (4.8.3.2) */
	gint		 tzofffrom;	/* Time Zone Offset From (4.8.3.3) */
	gint		 tzoffto;	/* Time Zone Offset To (4.8.3.4) */
	gchar		*tzurl;		/* Time Zone URL (4.8.3.5) */

	/* Relationship Properties (RFC 2445, Section 4.8.4) */

	GList		*attendees;	/* Attendee (4.8.4.1) */
	gchar		*contact;	/* Contact (4.8.4.2) */
	gchar		*uid;		/* Unique Identifier (4.8.4.7) */

	/* Recurrence Properties (RFC 2445, Section 4.8.5) */

	MIMEDirRecurrence *recurrence;	/* Recurrence Rule (4.8.5.4) */
	
	/* Alarm Properties (RFC 2445, Section 4.8.6) */

	gchar		*action;	/* Action (4.8.6.1) */
	guint		 repeat;	/* Repeat Count (4.8.6.2) */
	gint		 trigger;	/* Trigger (4.8.6.3) */
	MIMEDirDateTime *trigger_dt;
	gboolean	 trigger_end;

	/* Change Management Properties (RFC 2445, Section 4.8.7) */

	MIMEDirDateTime	*created;	/* Date/Time Created (4.8.7.1) */
	MIMEDirDateTime	*dtstamp;	/* Date/Time Stamp (4.8.7.2) */
	MIMEDirDateTime *last_modified;	/* Date/Time Last-modified (4.8.7.3) */
	gint		 sequence;	/* Sequence Number (4.8.7.4) */

	/* Custom Properties */

	gboolean	 allday;
};

static GObjectClass *parent_class = NULL;

static gint mimedir_vcomponent_signals[SIGNAL_LAST] = { 0, 0 };

/*
 * Class and Object Management
 */

GType
mimedir_vcomponent_get_type (void)
{
	static GType mimedir_vcomponent_type = 0;

	if (!mimedir_vcomponent_type) {
		static const GTypeInfo mimedir_vcomponent_info = {
			sizeof (MIMEDirVComponentClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcomponent_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVComponent),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcomponent_init,
		};

		mimedir_vcomponent_type = g_type_register_static (G_TYPE_OBJECT,
								  "MIMEDirVComponent",
								  &mimedir_vcomponent_info,
								  0);
	}

	return mimedir_vcomponent_type;
}


static void
mimedir_vcomponent_class_init (MIMEDirVComponentClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_vcomponent_dispose;
	gobject_class->set_property = mimedir_vcomponent_set_property;
	gobject_class->get_property = mimedir_vcomponent_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcomponent_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVComponentClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	mimedir_vcomponent_signals[SIGNAL_ALARMS_CHANGED] =
		g_signal_new ("alarms-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVComponentClass, alarms_changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_pointer ("category-list",
				      _("Category list"),
				      _("List of all categories of this component. This is a GList *, where the elements are gchar *'s"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CATEGORY_LIST, pspec);
	pspec = g_param_spec_pointer ("comment-list",
				      _("Comment list"),
				      _("List of all additional comments for this component. This is a GList *, where the elements are gchar *'s"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_COMMENT_LIST, pspec);
	pspec = g_param_spec_string ("description",
				     _("Description"),
				     _("Full description of this component"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DESCRIPTION, pspec);
	pspec = g_param_spec_uint ("percent-complete",
				   _("Percent complete"),
				   _("Percentage of this task's completion"),
				   0,
				   MIMEDIR_VCOMPONENT_PERCENT_NONE,
				   MIMEDIR_VCOMPONENT_PERCENT_NONE,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PERCENT_COMPLETE, pspec);
	pspec = g_param_spec_uint ("priority",
				   _("Priority"),
				   _("This task's priority"),
				   0, 9, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PRIORITY, pspec);
	pspec = g_param_spec_pointer ("resource-list", /* FIXME: whoops, this isn't handled */
				      _("Resource list"),
				      _("List of the resources assigned to this component"), /* FIXME: describe type */
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_RESOURCE_LIST, pspec);
	pspec = g_param_spec_uint ("status",
				   _("Status"),
				   _("This task's completion status"),
				   MIMEDIR_STATUS_UNKNOWN,
				   MIMEDIR_STATUS_FINAL,
				   MIMEDIR_STATUS_UNKNOWN,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_STATUS, pspec);
	pspec = g_param_spec_string ("summary",
				     _("Summary"),
				     _("Component's short description"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SUMMARY, pspec);

	pspec = g_param_spec_object ("dtcompleted",
				     _("Date/time completed"),
				     _("Date/time that this task was completed"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DTCOMPLETED, pspec);
	pspec = g_param_spec_object ("dtend",
				     _("Date/time end"),
				     _("End date/time of this component"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DTEND, pspec);
	pspec = g_param_spec_object ("due",
				     _("Due"),
				     _("This task's due date"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DUE, pspec);
	pspec = g_param_spec_object ("dtstart",
				     _("Date/time start"),
				     _("Start date/time of this component"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DTSTART, pspec);
	pspec = g_param_spec_uint ("duration",
				   _("Duration"),
				   _("This date's duration"),
				   0, G_MAXINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DURATION, pspec);
	pspec = g_param_spec_pointer ("freebusy",
				      _("Free/busy"),
				      _("GList of free/busy objects"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FREEBUSY, pspec);

	pspec = g_param_spec_string ("tzid",
				     _("Time zone identifier"),
				     _("This component's time zone identifier"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZID, pspec);
	pspec = g_param_spec_pointer ("tzname-list",
				      _("Time zone name list"),
				      _("GList of time zone names"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZNAME_LIST, pspec);
	pspec = g_param_spec_int ("tzoffsetfrom",
				  _("Time zone offset from"),
				  _("Time zone offset from"),
				  -24 * 60,
				   24 * 60,
				  0,
				  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZOFFFROM, pspec);
	pspec = g_param_spec_int ("tzoffsetto",
				  _("Time zone offset to"),
				  _("Time zone offset to"),
				  -24 * 60,
				   24 * 60,
				  0,
				  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZOFFTO, pspec);
	pspec = g_param_spec_string ("tzurl",
				     _("Time zone URL"),
				     _("URL to a time zone description"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TZURL, pspec);

	pspec = g_param_spec_string ("contact",
				     _("Contact"),
				     _("Contact information"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CONTACT, pspec);

	pspec = g_param_spec_string ("uid",
				     _("Unique identifier"),
				     _("A unique identifier for this component"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_UID, pspec);

	pspec = g_param_spec_object ("recurrence",
				     _("Recurrence object"),
				     _("An object representing the RRULE tag"),
				     MIMEDIR_TYPE_RECURRENCE,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_RECURRENCE, pspec);
	
	pspec = g_param_spec_string ("action",
				     _("Action"),
				     _("Action"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ACTION, pspec);
	pspec = g_param_spec_uint ("repeat",
				   _("Repeat"),
				   _("Repeat"),
				   0, G_MAXINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_REPEAT, pspec);
	pspec = g_param_spec_int ("trigger",
				  _("Trigger"),
				  _("Trigger"),
				  G_MININT, G_MAXINT, 0,
				  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TRIGGER, pspec);
	pspec = g_param_spec_object ("trigger-datetime",
				     _("Trigger date/time"),
				     _("Date/time of the trigger"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TRIGGER_DT, pspec);
	pspec = g_param_spec_boolean ("trigger-end",
				      _("Trigger end"),
				      _("Whether the trigger is set at the end of the time interval"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TRIGGER_END, pspec);

	pspec = g_param_spec_object ("created",
				     _("Created"),
				     _("Date/time of the creation"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CREATED, pspec);
	pspec = g_param_spec_object ("dtstamp",
				     _("Date/time stamp"),
				     _("Date/time of the creation of the iCalendar object"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DTSTAMP, pspec);
	pspec = g_param_spec_object ("last_modified",
				     _("Last modified"),
				     _("Date/time of the last modification"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LAST_MODIFIED, pspec);
	pspec = g_param_spec_uint ("sequence",
				   _("Sequence"),
				   _("Sequence number"),
				   0, G_MAXINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SEQ, pspec);
}


static void
mimedir_vcomponent_init (MIMEDirVComponent *vcomponent)
{
	MIMEDirVComponentPriv *priv;

	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	priv = g_new0 (MIMEDirVComponentPriv, 1);
	vcomponent->priv = priv;

	priv->changed     = FALSE;

	priv->md_class    = MIMEDIR_CLASS_PUBLIC;
	priv->latitude    = 0.0;
	priv->longitude   = 0.0;
	priv->percent     = -1;
	priv->priority    = 0;
	priv->status      = MIMEDIR_STATUS_UNKNOWN;

	priv->sequence    = -1;

	priv->opaque      = TRUE;

	priv->trigger     = 0;
	priv->trigger_end = FALSE;
}


static void
mimedir_vcomponent_dispose (GObject *object)
{
	MIMEDirVComponent     *vcomponent;
	MIMEDirVComponentPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (object));

	vcomponent = MIMEDIR_VCOMPONENT (object);

	priv = vcomponent->priv;

	if (priv)
		mimedir_vcomponent_clear (vcomponent);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
mimedir_vcomponent_set_property (GObject	*object,
				 guint		 property_id,
				 const GValue	*value,
				 GParamSpec	*pspec)
{
	MIMEDirVComponent *vcomponent;
	MIMEDirVComponentPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (object));

	vcomponent = MIMEDIR_VCOMPONENT (object);
	priv = vcomponent->priv;

	switch (property_id) {
	case PROP_CATEGORY_LIST:
		mimedir_utils_free_string_list (priv->categories);
		priv->categories = mimedir_utils_copy_string_list ((GList *) g_value_get_pointer (value));
		break;
	case PROP_COMMENT_LIST:
		mimedir_utils_free_string_list (priv->comment);
		priv->comment = mimedir_utils_copy_string_list ((GList *) g_value_get_pointer (value));
		break;
	case PROP_DESCRIPTION:
		mimedir_utils_free_string_list (priv->comment);
		priv->description = g_strdup (g_value_get_string (value));
		break;
	case PROP_PERCENT_COMPLETE: {
		guint percent;

		percent = g_value_get_uint (value);
		if (percent > 100)
			priv->percent = (gint) percent;
		else
			priv->percent = -1;
	}
	case PROP_PRIORITY:
		priv->priority = g_value_get_uint (value);
		break;
	case PROP_STATUS:
		priv->status = g_value_get_uint (value);
		break;
	case PROP_SUMMARY:
		mimedir_utils_set_property_string (&priv->summary, value);
		break;

	case PROP_DTCOMPLETED:
		if (priv->dtcompleted)
			g_object_unref (G_OBJECT (priv->dtcompleted));
		priv->dtcompleted = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->dtcompleted));
		break;
	case PROP_DTEND:
		if (priv->dtend)
			g_object_unref (G_OBJECT (priv->dtend));
		priv->dtend = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->dtend));
		break;
	case PROP_DUE:
		if (priv->due)
			g_object_unref (G_OBJECT (priv->due));
		priv->due = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->due));
		break;
	case PROP_DTSTART:
		if (priv->dtstart)
			g_object_unref (G_OBJECT (priv->dtstart));
		priv->dtstart = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->dtstart));
		break;
	case PROP_DURATION:
		priv->duration = g_value_get_uint (value);
		break;
	case PROP_FREEBUSY:
		mimedir_utils_free_object_list (priv->freebusy);
		priv->freebusy = mimedir_utils_copy_object_list ((GList *) g_value_get_pointer (value));
		break;

	case PROP_TZID:
		mimedir_utils_set_property_string (&priv->tzid, value);
		break;
	case PROP_TZNAME_LIST:
		mimedir_utils_free_string_list (priv->tznames);
		priv->tznames = mimedir_utils_copy_string_list ((GList *) g_value_get_pointer (value));
		break;
	case PROP_TZOFFFROM:
		priv->tzofffrom = g_value_get_int (value);
		break;
	case PROP_TZOFFTO:
		priv->tzoffto = g_value_get_int (value);
		break;
	case PROP_TZURL:
		mimedir_utils_set_property_string (&priv->tzurl, value);
		break;

	case PROP_CONTACT:
		mimedir_utils_set_property_string (&priv->contact, value);
		break;

	case PROP_UID:
		mimedir_utils_set_property_string (&priv->uid, value);
		break;

	case PROP_RECURRENCE:
		if (priv->recurrence)
			g_object_unref (G_OBJECT (priv->recurrence));
		priv->recurrence = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->recurrence));
		break;

	case PROP_ACTION:
		mimedir_utils_set_property_string (&priv->action, value);
		break;
	case PROP_REPEAT:
		priv->repeat = g_value_get_uint (value);
		break;
	case PROP_TRIGGER:
		priv->trigger = g_value_get_int (value);
		break;
	case PROP_TRIGGER_DT:
		if (priv->trigger_dt)
			g_object_unref (G_OBJECT (priv->trigger_dt));
		priv->trigger_dt = g_value_get_object (value);
		g_object_ref (G_OBJECT (priv->trigger_dt));
		break;
	case PROP_TRIGGER_END:
		priv->trigger_end = g_value_get_boolean (value);
		break;

	case PROP_CREATED: {
		MIMEDirDateTime *dt;

		dt = MIMEDIR_DATETIME (g_value_get_object (value));

		g_return_if_fail ((dt->flags & MIMEDIR_DATETIME_DATE) &&
				  (dt->flags & MIMEDIR_DATETIME_TIME));
		g_return_if_fail (dt->timezone == MIMEDIR_DATETIME_UTC);

		if (priv->created)
			g_object_unref (G_OBJECT (priv->created));
		priv->created = dt;
		g_object_ref (G_OBJECT (priv->created));
		break;
	}
	case PROP_DTSTAMP: {
		MIMEDirDateTime *dt;

		dt = MIMEDIR_DATETIME (g_value_get_object (value));

		g_return_if_fail ((dt->flags & MIMEDIR_DATETIME_DATE) &&
				  (dt->flags & MIMEDIR_DATETIME_TIME));
		g_return_if_fail (dt->timezone == MIMEDIR_DATETIME_UTC);

		if (priv->dtstamp)
			g_object_unref (G_OBJECT (priv->dtstamp));
		priv->dtstamp = dt;
		g_object_ref (G_OBJECT (priv->dtstamp));
		break;
	}
	case PROP_LAST_MODIFIED: {
		MIMEDirDateTime *dt;

		dt = MIMEDIR_DATETIME (g_value_get_object (value));

		g_return_if_fail ((dt->flags & MIMEDIR_DATETIME_DATE) &&
				  (dt->flags & MIMEDIR_DATETIME_TIME));
		g_return_if_fail (dt->timezone == MIMEDIR_DATETIME_UTC);

		if (priv->last_modified)
			g_object_unref (G_OBJECT (priv->last_modified));
		priv->last_modified = dt;
		g_object_ref (G_OBJECT (priv->last_modified));
		break;
	}
	case PROP_SEQ:
		priv->sequence = g_value_get_uint (value);
	break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	mimedir_vcomponent_changed (vcomponent);
}


static void
mimedir_vcomponent_get_property (GObject	*object,
			    guint	 property_id,
			    GValue	*value,
			    GParamSpec	*pspec)
{
	MIMEDirVComponent *vcomponent;
	MIMEDirVComponentPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (object));

	vcomponent = MIMEDIR_VCOMPONENT (object);
	priv = vcomponent->priv;

	switch (property_id) {
	case PROP_CATEGORY_LIST:
		g_value_set_pointer (value, priv->categories);
		break;
	case PROP_COMMENT_LIST:
		g_value_set_pointer (value, priv->comment);
		break;
	case PROP_DESCRIPTION:
		g_value_set_string (value, priv->description);
		break;
	case PROP_PERCENT_COMPLETE:
		if (priv->percent == -1)
			g_value_set_uint (value, MIMEDIR_VCOMPONENT_PERCENT_NONE);
		else
			g_value_set_uint (value, (guint) priv->percent);
		break;
	case PROP_PRIORITY:
		g_value_set_uint (value, priv->priority);
		break;
	case PROP_STATUS:
		g_value_set_uint (value, priv->status);
		break;
	case PROP_SUMMARY:
		g_value_set_string (value, priv->summary ? priv->summary : "");
		break;

	case PROP_DTCOMPLETED:
		if (priv->dtcompleted)
			g_value_set_object (value, G_OBJECT (priv->dtcompleted));
		break;
	case PROP_DTEND:
		if (priv->dtend)
			g_value_set_object (value, G_OBJECT (priv->dtend));
		break;
	case PROP_DUE:
		if (priv->due)
			g_value_set_object (value, G_OBJECT (priv->due));
		break;
	case PROP_DTSTART:
		if (priv->dtstart)
			g_value_set_object (value, G_OBJECT (priv->dtstart));
		break;
	case PROP_DURATION:
		g_value_set_uint (value, priv->duration);
		break;
	case PROP_FREEBUSY:
		g_value_set_pointer (value, priv->freebusy);
		break;

	case PROP_TZID:
		g_value_set_string (value, priv->tzid);
		break;
	case PROP_TZNAME_LIST:
		g_value_set_pointer (value, priv->tznames);
		break;
	case PROP_TZOFFFROM:
		g_value_set_int (value, priv->tzofffrom);
		break;
	case PROP_TZOFFTO:
		g_value_set_int (value, priv->tzoffto);
		break;
	case PROP_TZURL:
		g_value_set_string (value, priv->tzurl);
		break;

	case PROP_CONTACT:
		g_value_set_string (value, priv->contact);
		break;

	case PROP_UID:
		g_value_set_string (value, priv->uid);
		break;

	case PROP_RECURRENCE:
		if (priv->recurrence)
			g_value_set_object (value, G_OBJECT (priv->recurrence));
		break;

	case PROP_ACTION:
		g_value_set_string (value, priv->action);
		break;
	case PROP_REPEAT:
		g_value_set_uint (value, priv->repeat);
		break;
	case PROP_TRIGGER:
		g_value_set_int (value, priv->trigger);
		break;
	case PROP_TRIGGER_DT:
		if (priv->trigger_dt)
			g_value_set_object (value, priv->trigger_dt);
		break;
	case PROP_TRIGGER_END:
		g_value_set_boolean (value, priv->trigger_end);
		break;

	case PROP_SEQ:
		g_value_set_uint (value, (guint) MAX (priv->sequence, 0));
		break;

	case PROP_CREATED:
		if (priv->created)
			g_value_set_object (value, priv->created);
		break;
	case PROP_DTSTAMP:
		if (priv->dtstamp)
			g_value_set_object (value, priv->dtstamp);
		break;
	case PROP_LAST_MODIFIED:
		if (priv->last_modified)
			g_value_set_object (value, priv->last_modified);
		break;

 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Utilities
 */

#define SECS_PER_MINUTE (60)
#define SECS_PER_HOUR (SECS_PER_MINUTE * 60)
#define SECS_PER_DAY (SECS_PER_HOUR * 24)
#define SECS_PER_WEEK (SECS_PER_DAY * 7)

static void
mimedir_vcomponent_set_value_duration (MIMEDirAttribute *attr, gint duration)
{
	GString *s;

	s = g_string_new ("");

	if (duration < 0) {
		g_string_append (s, "-");
		duration = -duration;
	}
	g_string_append (s, "P");

	if (duration == 0)
		g_string_append (s, "0D");
	else if (duration % SECS_PER_WEEK == 0)
		g_string_append_printf (s, "%dW", duration / SECS_PER_WEEK);
	else {
		if (duration >= SECS_PER_DAY) {
			g_string_append_printf (s, "%dD", duration / SECS_PER_DAY);
			duration %= SECS_PER_DAY;
		}
		if (duration > 0) {
			g_string_append_printf (s, "T%dH", duration / SECS_PER_HOUR);
			duration %= SECS_PER_HOUR;
		}
		if (duration > 0) {
			g_string_append_printf (s, "%dM", duration / SECS_PER_MINUTE);
			duration %= SECS_PER_MINUTE;
		}
		if (duration > 0)
			g_string_append_printf (s, "%dS", duration);
	}

	mimedir_attribute_set_value (attr, s->str);

	g_string_free (s, TRUE);
}


static guint
mimedir_vcomponent_parse_number (const gchar *name, const gchar *type, const gchar **s, GError **error)
{
	guint num = 0;

	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	if (**s < '0' || **s > '9') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, type, name);
		return 0;
	}

	while (**s >= '0' && **s <= '9') {
		num *= 10;
		num += **s - '0';
		(*s)++;
	}

	return num;
}


static gint
mimedir_vcomponent_get_value_duration (MIMEDirAttribute *attr, GError **error)
{
	const gchar *name;
	const gchar *s;
	gint duration = 0;
	gboolean neg = FALSE;
	gboolean dur_week = FALSE;

	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	name = mimedir_attribute_get_name (attr);

	s = mimedir_attribute_get_value (attr);

	if (s[0] == '-') {
		neg = TRUE;
		s++;
	} else if (s[0] == '+')
		s++;

	if (s[0] != 'P' && s[0] != 'p') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "DURATION", name);
		return 0;
	}
	s++;

	if (s[0] != 'T' && s[0] != 't') {
		GError *err = NULL;
		guint num;

		num = mimedir_vcomponent_parse_number (name, "DURATION", &s, &err);
		if (err) {
			g_propagate_error (error, err);
			return 0;
		}

		if (s[0] == 'D' || s[0] == 'd')
			duration = num * SECS_PER_DAY;
		else if (s[0] == 'W' || s[0] == 'w') {
			duration = num * SECS_PER_WEEK;
			dur_week = TRUE;
		} else {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "DURATION", name);
			return 0;
		}
		s++;
	}

	if ((s[0] == 'T' || s[0] == 't') && !dur_week) {
	  /* Parse a dur-time */
		GError *err = NULL;
		guint num;

		s++;

		/* Must begin with a number */
		num = mimedir_vcomponent_parse_number (name, "DURATION", &s, &err);
		if (err) {
			g_propagate_error (error, err);
			return 0;
		}
		/* Is it dur-hour? */
		if (s[0] == 'H') {
		  s++;
		  duration += num * SECS_PER_HOUR;

		  /* Got dur-hour, any more? */
		  if (s[0]) {
			num = mimedir_vcomponent_parse_number (name, "DURATION", &s, &err);
			if (err) {
				g_propagate_error (error, err);
				return 0;
			}
		  }
		}

		/* Is it dur-minute? */
		if (s[0] == 'M') {
			s++;
			duration += num * SECS_PER_MINUTE;

			/* Got dur-minute, any more? */
			if (s[0]) {
			  num = mimedir_vcomponent_parse_number (name, "DURATION", &s, &err);
			  if (err) {
				g_propagate_error (error, err);
				return 0;
			  }
			}
		}

		/* Is it dur-second? */
		if (s[0] == 'S') {
			s++;
			duration += num;
		}
	}

	if (s[0] != '\0') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "DURATION", name);
		return 0;
	}

	return neg ? -duration : duration;
}

static void
mimedir_vcomponent_set_value_utcoffset (MIMEDirAttribute *attr, gint offset)
{
	gchar *s;
	gchar sign;
	guint hour, minute;

	sign = offset < 0 ? '-' : '+';
	if (offset < 0)
		offset = -offset;
	hour = (guint) offset / 100;
	minute = (guint) offset % 100;

	g_return_if_fail (hour < 24);
	g_return_if_fail (minute < 60);

	s = g_strdup_printf ("%c%02d%02d", sign, hour, minute);
	mimedir_attribute_set_value (attr, s);
	g_free (s);
}

static gint
mimedir_vcomponent_get_value_utcoffset (MIMEDirAttribute *attr, GError **error)
{
	const gchar *name, *s;
	gsize len;
	gboolean negative;
	guint hour, minute, result;
	gint i;

	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	name = mimedir_attribute_get_name (attr);

	s = mimedir_attribute_get_value (attr);

	len = strlen (s);

	if (len != 5 && len != 7) {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "UTC-OFFSET", name);
		return 0;
	}

	switch (s[0]) {
	case '+':
		negative = FALSE;
		break;
	case '-':
		negative = TRUE;
		break;
	default:
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "UTC-OFFSET", name);
		return 0;
	}

	for (i = 1; i < len; i++) {
		if (s[i] < '0' || s[i] > '9') {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "UTC-OFFSET", name);
			return 0;
		}
	}

	hour   = (s[1] - '0') * 10 + s[2] - '0';
	minute = (s[3] - '0') * 10 + s[4] - '0';

	/* Ignore seconds */

	if (hour > 23 || minute > 59) {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
		return 0;
	}

	result = hour * 100 + minute;

	return negative ? -result : result;
}

/*
 * Private Methods
 */

/* Free VComponent's allocated memory */
static void
mimedir_vcomponent_clear (MIMEDirVComponent *vcomponent)
{
	MIMEDirVComponentPriv *priv;

	priv = vcomponent->priv;

	if (!priv)
		return;

	mimedir_utils_free_object_list (priv->alarms);

	if (priv->recurrence)
		g_object_unref (G_OBJECT (priv->recurrence));

	mimedir_utils_free_object_list (priv->attachments);
	mimedir_utils_free_string_list (priv->categories);
	g_free (priv->md_class_str);
	mimedir_utils_free_string_list (priv->comment);
	g_free (priv->description);
	g_free (priv->location);
	g_free (priv->location_alt);
	mimedir_utils_free_string_list (priv->resources);
	g_free (priv->summary);
	g_free (priv->uid);

	if (priv->dtcompleted)
		g_object_unref (G_OBJECT (priv->dtcompleted));
	if (priv->dtstart)
		g_object_unref (G_OBJECT (priv->dtstart));
	if (priv->dtend)
		g_object_unref (G_OBJECT (priv->dtend));
	if (priv->due)
		g_object_unref (G_OBJECT (priv->due));
	mimedir_utils_free_object_list (priv->freebusy);

	g_free (priv->tzid);
	mimedir_utils_free_string_list (priv->tznames);
	g_free (priv->tzurl);

	mimedir_utils_free_object_list (priv->attendees);

	if (priv->trigger_dt)
		g_object_unref (G_OBJECT (priv->trigger_dt));

	if (priv->created)
		g_object_unref (G_OBJECT (priv->created));
	if (priv->dtstamp)
		g_object_unref (G_OBJECT (priv->dtstamp));
	if (priv->last_modified)
		g_object_unref (G_OBJECT (priv->last_modified));

	g_free (priv);
	vcomponent->priv = NULL;
}


/* Reallocate memory for the VComponent */
static void
mimedir_vcomponent_reset (MIMEDirVComponent *vcomponent)
{
	mimedir_vcomponent_clear (vcomponent);
	mimedir_vcomponent_init  (vcomponent);
}


/* Emit the changed signal */
static void
mimedir_vcomponent_changed (MIMEDirVComponent *vcomponent)
{
	vcomponent->priv->changed = TRUE;
	g_signal_emit (G_OBJECT (vcomponent), mimedir_vcomponent_signals[SIGNAL_CHANGED], 0);
}


static gboolean
parse_rrule (MIMEDirVComponent *vcomponent, MIMEDirAttribute *attr, GError **error)
{
	MIMEDirVComponentPriv *priv = vcomponent->priv;

	priv->recurrence = mimedir_recurrence_new_parse (attr, error);

	return priv->recurrence != NULL;
}

static gboolean
mimedir_vcomponent_parse_attribute (MIMEDirVComponent *vcomponent, MIMEDirAttribute *attr, GError **error)
{
	GError *err = NULL;
	MIMEDirVComponentPriv *priv;
	const gchar *name;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = vcomponent->priv;

	name = mimedir_attribute_get_name (attr);

	/*
	 * Generic Types (RFC 2425, Section 6)
	 */

	/* Profile Begin (6.4) */

	if (!g_ascii_strcasecmp (name, "BEGIN")) {
		/* ignore */
	}

	/* Profile End (6.5) */

	else if (!g_ascii_strcasecmp (name, "END")) {
		/* ignore */
	}

	/*
	 * Descriptive Properties (RFC 2445, Section 4.8.1)
	 */

	/* Attachment (4.8.1.1) */

	else if (!g_ascii_strcasecmp (name, "ATTACH")) {
		MIMEDirAttachment *attachment;

		attachment = mimedir_attachment_new_from_attribute (attr, error);
		if (!attachment)
			return FALSE;

		priv->attachments = g_list_append (priv->attachments, attachment);
	}

	/* Categories (4.8.1.2) */

	else if (!g_ascii_strcasecmp (name, "CATEGORIES")) {
		GSList *l, *i;

		l = mimedir_attribute_get_value_text_list (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		for (i = l; i != NULL; i = g_slist_next (i))
			priv->categories = g_list_append (priv->categories, g_strdup (i->data));

		mimedir_attribute_free_text_list (l);
	}

	/* Classification (4.8.1.3) */

	else if (!g_ascii_strcasecmp (name, "CLASS")) {
		const gchar *v;

		if (priv->md_class != MIMEDIR_CLASS_PUBLIC) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		v = mimedir_attribute_get_value (attr);
		if (!g_ascii_strcasecmp (v, "PUBLIC"))
			priv->md_class = MIMEDIR_CLASS_PUBLIC;
		else if (!g_ascii_strcasecmp (v, "PRIVATE"))
			priv->md_class = MIMEDIR_CLASS_PRIVATE;
		else if (!g_ascii_strcasecmp (v, "CONFIDENTIAL"))
			priv->md_class = MIMEDIR_CLASS_CONFIDENTIAL;
		else {
			priv->md_class = MIMEDIR_CLASS_CUSTOM;
			priv->md_class_str = g_strdup (v);
		}
	}

	/* Comment (4.8.1.4) */

	else if (!g_ascii_strcasecmp (name, "COMMENT")) {
		gchar *text;

		text = mimedir_attribute_get_value_text (attr, error);
		if (!text)
			return FALSE;

		priv->comment = g_list_append (priv->comment, text);
	}

	/* Description (4.8.1.5) */

	else if (!g_ascii_strcasecmp (name, "DESCRIPTION")) {
		if (priv->description) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->description = mimedir_attribute_get_value_text (attr, error);
		if (!priv->description)
			return FALSE;
	}

	/* Geographic Position (4.8.1.6) */

	else if (!g_ascii_strcasecmp (name, "GEO")) {
		GSList *list;

		if (priv->latitude != 0.0 || priv->longitude != 0.0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		list = mimedir_attribute_get_value_float_list (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		if (g_slist_length (list) != 2) {
			mimedir_attribute_free_float_list (list);
			if (g_slist_length (list) < 2)
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT_STR, name);
			else
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG_STR, name);
			return FALSE;
		}

		priv->latitude  = *(gdouble *)list->data;
		priv->longitude = *(gdouble *)list->next->data;

		mimedir_attribute_free_float_list (list);
	}

	/* Location (4.8.1.7) */

	else if (!g_ascii_strcasecmp (name, "LOCATION")) {
		const gchar *altrep;

		if (priv->location || priv->location_alt) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->location = mimedir_attribute_get_value_text (attr, error);
		if (!priv->location)
			return FALSE;

		altrep = mimedir_attribute_get_parameter_value (attr, "altrep");
		if (altrep)
			priv->location_alt = g_strdup (altrep);
	}

	/* Percent Complete (4.8.1.8) */

	else if (!g_ascii_strcasecmp (name, "PERCENT-COMPLETE")) {
		gint percent;
		if (priv->percent >= 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		percent = mimedir_attribute_get_value_int (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		if (percent < 0 || percent > 100) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}

		priv->percent = percent;
	}

	/* Priority (4.8.1.9) */

	else if (!g_ascii_strcasecmp (name, "PRIORITY")) {
		gint i;

		i = mimedir_attribute_get_value_int (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		if (i < 0 || i > 9) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}

		priv->priority = i;
	}

	/* Resources (4.8.1.10) */

	else if (!g_ascii_strcasecmp (name, "RESOURCES")) {
		GSList *slist, *node;
		GList *list = NULL;

		slist = mimedir_attribute_get_value_text_list (attr, error);
		for (node = slist; node != NULL; node = g_slist_next (node)) {
			g_assert (node->data != NULL);
			list = g_list_prepend (list, g_strdup (node->data));
		}
		list = g_list_reverse (list);
		list = g_list_concat (priv->resources, list);
		priv->resources = list;

		mimedir_attribute_free_text_list (slist);
	}

	/* Status (4.8.1.11) */

	else if (!g_ascii_strcasecmp (name, "STATUS")) {
		gchar *status;

		if (priv->status != MIMEDIR_STATUS_UNKNOWN) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		status = mimedir_attribute_get_value_text (attr, error);
		if (!status)
			return FALSE;

		if (!g_ascii_strcasecmp (status, "CANCELLED"))
			priv->status = MIMEDIR_STATUS_CANCELLED;
		else if (!g_ascii_strcasecmp (status, "TENTATIVE"))
			priv->status = MIMEDIR_STATUS_TENTATIVE;
		else if (!g_ascii_strcasecmp (status, "CONFIRMED"))
			priv->status = MIMEDIR_STATUS_CONFIRMED;
		else if (!g_ascii_strcasecmp (status, "NEEDS-ACTION") ||
			 !g_ascii_strcasecmp (status, "NEEDS ACTION"))
			priv->status = MIMEDIR_STATUS_NEEDS_ACTION;
		else if (!g_ascii_strcasecmp (status, "COMPLETED"))
			priv->status = MIMEDIR_STATUS_COMPLETED;
		else if (!g_ascii_strcasecmp (status, "IN-PROCESS") ||
			 !g_ascii_strcasecmp (status, "IN PROCESS"))
			priv->status = MIMEDIR_STATUS_IN_PROCESS;
		else if (!g_ascii_strcasecmp (status, "DRAFT"))
			priv->status = MIMEDIR_STATUS_DRAFT;
		else if (!g_ascii_strcasecmp (status, "FINAL"))
			priv->status = MIMEDIR_STATUS_FINAL;
		/* NOT SPECIFIED is not a legal STATUS according to RFC2445
		   but it is generated by the Remote Calendars plugin for Outlook
		   (https://sourceforge.net/projects/remotecalendars/) so we ignore it */
		else if (!g_ascii_strcasecmp (status, "NOT SPECIFIED"))
			priv->status = MIMEDIR_STATUS_UNKNOWN;
		else {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
	}

	/* Summary (4.8.1.12) */

	else if (!g_ascii_strcasecmp (name, "SUMMARY")) {
		if (priv->summary) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->summary = mimedir_attribute_get_value_text (attr, error);
		if (!priv->summary)
			return FALSE;
	}

	/*
	 * Date and Time Properties (RFC 2445, Section 4.8.2)
	 */

	/* Date/Time Completed (4.8.2.1) */

	else if (!g_ascii_strcasecmp (name, "COMPLETED")) {
		if (priv->dtcompleted) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->dtcompleted = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->dtcompleted)
			return FALSE;
		if (!(priv->dtcompleted->flags & MIMEDIR_DATETIME_DATE)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
			return FALSE;
		}
	}

	/* Date/Time End (4.8.2.2) */

	else if (!g_ascii_strcasecmp (name, "DTEND")) {
		if (priv->dtend) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->dtend = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->dtend)
			return FALSE;
		if (!(priv->dtend->flags & MIMEDIR_DATETIME_DATE)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
			return FALSE;
		}

		if (mimedir_attribute_has_parameter (attr, "TZID")) {
			const gchar *tzid;

			tzid = mimedir_attribute_get_parameter_value (attr, "TZID");
			g_object_set (G_OBJECT (priv->dtend),
				      "tzid", tzid,
				      NULL);
		}
	}

	/* Date/Time Due (4.8.2.3) */

	else if (!g_ascii_strcasecmp (name, "DUE")) {
		if (priv->due) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->due = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->due)
			return FALSE;
		if (!(priv->due->flags & MIMEDIR_DATETIME_DATE)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
			return FALSE;
		}

		if (mimedir_attribute_has_parameter (attr, "TZID")) {
			const gchar *tzid;

			tzid = mimedir_attribute_get_parameter_value (attr, "TZID");
			g_object_set (G_OBJECT (priv->due),
				      "tzid", tzid,
				      NULL);
		}
	}

	/* Date/Time Start (4.8.2.4) */

	else if (!g_ascii_strcasecmp (name, "DTSTART")) {
		if (priv->dtstart) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->dtstart = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->dtstart)
			return FALSE;
		if (!(priv->dtstart->flags & MIMEDIR_DATETIME_DATE)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
			return FALSE;
		}

		if (mimedir_attribute_has_parameter (attr, "TZID")) {
			const gchar *tzid;

			tzid = mimedir_attribute_get_parameter_value (attr, "TZID");
			g_object_set (G_OBJECT (priv->dtstart),
				      "tzid", tzid,
				      NULL);
		}
	}

	/* Duration (4.8.2.5) */

	else if (!g_ascii_strcasecmp (name, "DURATION")) {
		gint i;

		if (priv->duration != 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		i = mimedir_vcomponent_get_value_duration (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		if (i < 0) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}

		priv->duration = i;
	}

	/* Free/Busy Time (4.8.2.6) */

	else if (!g_ascii_strcasecmp (name, "FREEBUSY")) {
		GList *list = NULL;
		GSList *slist, *node;
		MIMEDirVFreeBusyType fb = MIMEDIR_VFREEBUSY_BUSY;

		if (mimedir_attribute_has_parameter (attr, "FBTYPE")) {
			const gchar *s;

			s = mimedir_attribute_get_parameter_value (attr, "FBTYPE");
			if (!g_ascii_strcasecmp (s, "FREE"))
				fb = MIMEDIR_VFREEBUSY_FREE;
			else if (!g_ascii_strcasecmp (s, "BUSY"))
				fb = MIMEDIR_VFREEBUSY_BUSY;
			else if (!g_ascii_strcasecmp (s, "BUSY-UNAVAILABLE") ||
				 !g_ascii_strcasecmp (s, "BUSY UNAVAILABLE"))
				fb = MIMEDIR_VFREEBUSY_BUSY_UNAVAILABLE;
			else if (!g_ascii_strcasecmp (s, "BUSY-TENTATIVE") ||
				 !g_ascii_strcasecmp (s, "BUSY TENTATIVE"))
				fb = MIMEDIR_VFREEBUSY_BUSY_TENTATIVE;
		}

		slist = mimedir_attribute_get_value_text_list (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		for (node = slist; node != NULL; node = g_slist_next (node)) {
			MIMEDirPeriod *period;

			g_assert (node->data != NULL);

			period = mimedir_period_new_parse ((const gchar *) node->data);
			g_object_set (G_OBJECT (period),
				      "fbtype", fb,
				      NULL);

			list = g_list_prepend (list, period);
		}

		list = g_list_reverse (list);
		priv->freebusy = g_list_concat (priv->freebusy, list);

		mimedir_attribute_free_text_list (slist);
	}

	/* Time Transparency (4.8.2.7) */

	else if (!g_ascii_strcasecmp (name, "TRANSP")) {
		const gchar *s;

		s = mimedir_attribute_get_value (attr);

		if (!g_ascii_strcasecmp (s, "OPAQUE") ||
		    !g_ascii_strcasecmp (s, "0"))
			priv->opaque = TRUE;
		else if (!g_ascii_strcasecmp (s, "TRANSPARENT") ||
			 !g_ascii_strcasecmp (s, "1"))
			priv->opaque = FALSE;
		else {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
	}

	/*
	 * Time Zone Properties (RFC 2445, Section 4.8.3)
	 */

	/* Time Zone Identifier (4.8.3.1) */

	else if (!g_ascii_strcasecmp (name, "TZID")) {
		const gchar *s;

		if (priv->tzid) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		s = mimedir_attribute_get_value (attr);
		priv->tzid = g_strdup (s);
	}

	/* Time Zone Name (4.8.3.2) */

	else if (!g_ascii_strcasecmp (name, "TZNAME")) {
		gchar *s;

		s = mimedir_attribute_get_value_text (attr, error);
		if (!s)
			return FALSE;

		priv->tznames = g_list_append (priv->tznames, s);
	}

	/* Time Zone Offset From (4.8.3.3) */

	else if (!g_ascii_strcasecmp (name, "TZOFFSETFROM")) {
		if (priv->tzofffrom != 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->tzofffrom = mimedir_vcomponent_get_value_utcoffset (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}
	}

	/* Time Zone Offset To (4.8.3.4) */

	else if (!g_ascii_strcasecmp (name, "TZOFFSETTO")) {
		if (priv->tzoffto != 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->tzoffto = mimedir_vcomponent_get_value_utcoffset (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}
	}

	/* Time Zone URL (4.8.3.5) */

	else if (!g_ascii_strcasecmp (name, "TZURL")) {
		if (priv->tzurl) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->tzurl = mimedir_attribute_get_value_uri (attr, error);
		if (!priv->tzurl)
			return FALSE;
	}

	/*
	 * Relationship Properties (RFC 2445, Section 4.8.4)
	 */

	/* Attendee (4.8.4.1) */

	else if (!g_ascii_strcasecmp (name, "ATTENDEE")) {
		MIMEDirAttendee *att;

		att = mimedir_attendee_new_from_attribute (attr, error);
		if (!att)
			return FALSE;

		priv->attendees = g_list_prepend (priv->attendees, att);
	}

	/* Contact (4.8.4.2) */

	else if (!g_ascii_strcasecmp (name, "CONTACT")) {
		if (!priv->contact) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->contact = mimedir_attribute_get_value_text (attr, error);
	}

	/* Organizer (4.8.4.3) */

	else if (!g_ascii_strcasecmp (name, "ORGANIZER")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Recurrence ID (4.8.4.4) */

	else if (!g_ascii_strcasecmp (name, "RECURRENCE-ID")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Related To (4.8.4.5) */

	else if (!g_ascii_strcasecmp (name, "RELATED-TO")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Uniform Resource Locator (4.8.4.6) */

	else if (!g_ascii_strcasecmp (name, "URL")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Unique Identifier (4.8.4.7) */

	else if (!g_ascii_strcasecmp (name, "UID")) {
		if (priv->uid) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->uid = mimedir_attribute_get_value_text (attr, error);
		if (!priv->uid)
			return FALSE;
	}

	/*
	 * Recurrence Properties (RFC 2445, Section 4.8.5)
	 */

	/* Exception Date/Times (4.8.5.1) */

	else if (!g_ascii_strcasecmp (name, "EXDATE")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Exception Rule (4.8.5.2) */

	else if (!g_ascii_strcasecmp (name, "EXRULE")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Recurrence Date/Times (4.8.5.3) */

	else if (!g_ascii_strcasecmp (name, "RDATE")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/* Recurrence Rule (4.8.5.4) */

	else if (!g_ascii_strcasecmp (name, "RRULE")) {
		if (priv->recurrence) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		if (!parse_rrule (vcomponent, attr, error))
			return FALSE;
	}

	/*
	 * Alarm Properties (RFC 2445, Section 4.8.6)
	 */

	/* Action (4.8.6.1) */

	else if (!g_ascii_strcasecmp (name, "ACTION")) {
		if (priv->action) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->action = mimedir_attribute_get_value_text (attr, error);
		if (!priv->action)
			return FALSE;
	}

	/* Repeat Count (4.8.6.2) */

	else if (!g_ascii_strcasecmp (name, "REPEAT")) {
		gint i;

		if (priv->repeat > 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		i = mimedir_attribute_get_value_int (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		if (i < 0) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}

		priv->repeat = (guint) i;
	}

	/* Trigger (4.8.6.3) */

	else if (!g_ascii_strcasecmp (name, "TRIGGER")) {
		const gchar *par;

		if (priv->trigger != 0 ||
		    priv->trigger_dt   ||
		    priv->trigger_end) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->trigger_end = FALSE;

		par = mimedir_attribute_get_parameter_value (attr, "VALUE");
		if (par && !g_ascii_strcasecmp (par, "DATE-TIME")) {
			/* Note: RFC says that is DATE-TIME is specified, value must be in UTC */
			priv->trigger_dt = mimedir_attribute_get_value_datetime (attr, error);
			if (!priv->trigger_dt)
				return FALSE;
		} else {
			priv->trigger = mimedir_vcomponent_get_value_duration (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			par = mimedir_attribute_get_parameter_value (attr, "RELATED");
			if (par && !g_ascii_strcasecmp (par, "END"))
				priv->trigger_end = TRUE;
		}
	}

	/*
	 * Change Management Properties (RFC 2445, Section 4.8.7)
	 */

	/* Date/Time Created (4.8.7.1) */

	else if (!g_ascii_strcasecmp (name, "CREATED") ||
		 !g_ascii_strcasecmp (name, "DCREATED")) { /* compat */

		if (priv->created) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->created = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->created)
			return FALSE;

		if (!(priv->created->flags & MIMEDIR_DATETIME_DATE) ||
		    !(priv->created->flags & MIMEDIR_DATETIME_TIME)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
		if (priv->created->timezone != MIMEDIR_DATETIME_UTC)
			mimedir_datetime_to_utc (priv->created);
	}

	/* Date/Time Stamp (4.8.7.2) */

	else if (!g_ascii_strcasecmp (name, "DTSTAMP")) {

		if (priv->dtstamp) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->dtstamp = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->dtstamp)
			return FALSE;

		if (!(priv->dtstamp->flags & MIMEDIR_DATETIME_DATE) ||
		    !(priv->dtstamp->flags & MIMEDIR_DATETIME_TIME)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
		if (priv->dtstamp->timezone != MIMEDIR_DATETIME_UTC) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
	}

	/* Last Modified (4.8.7.3) */

	else if (!g_ascii_strcasecmp (name, "LAST-MODIFIED")) {
		if (priv->last_modified) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->last_modified = mimedir_attribute_get_value_datetime (attr, error);
		if (!priv->last_modified)
			return FALSE;

		if (!(priv->last_modified->flags & MIMEDIR_DATETIME_DATE) ||
		    !(priv->last_modified->flags & MIMEDIR_DATETIME_TIME)) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
			return FALSE;
		}
		/* RFC 2445 requires the LAST-MODIFIED field to be in UTC
		 * time format (section 4.8.7.3). Unfortunately some software
		 * does not conform to this requirement.
		 */
		if (priv->last_modified->timezone != MIMEDIR_DATETIME_UTC)
			mimedir_datetime_to_utc (priv->last_modified);
	}

	/* Sequence Number (4.8.7.4) */

	else if (!g_ascii_strcasecmp (name, "SEQUENCE")) {
		if (priv->sequence >= 0) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
			return FALSE;
		}

		priv->sequence = mimedir_attribute_get_value_int (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}
	}

	/*
	 * Miscellaneous Properties (RFC 2445, Section 4.8.8)
	 */

	/* Non-Standard Properties (4.8.8.1) handled below */

	/* Request Status (4.8.8.2) */

	else if (!g_ascii_strcasecmp (name, "REQUEST-STATUS")) {
		/* FIXME: ensure uniqueness */

		/* FIXME */
	}

	/*
	 * Custom Properties
	 */

	/* Organization Name (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "ORGNAME")) {
		/* FIXME */
	}

	/* Display Alarm (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "DALARM")) {
		MIMEDirVAlarm *alarm;
		GSList *list;

		list = mimedir_attribute_get_value_structured_text (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		alarm = mimedir_valarm_new (MIMEDIR_VALARM_DISPLAY);
		/* FIXME: anchor */
		priv->alarms = g_list_prepend (priv->alarms, alarm);

		mimedir_attribute_free_structured_text_list (list);
	}

	/* Audio Alarm (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "AALARM")) {
		MIMEDirVAlarm *alarm;
		GSList *list;

		list = mimedir_attribute_get_value_structured_text (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		alarm = mimedir_valarm_new (MIMEDIR_VALARM_AUDIO);
		/* FIXME: anchor */
		priv->alarms = g_list_prepend (priv->alarms, alarm);

		mimedir_attribute_free_structured_text_list (list);
	}

	/* Program Alarm (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "PALARM")) {
		MIMEDirVAlarm *alarm;
		GSList *list;

		list = mimedir_attribute_get_value_structured_text (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		alarm = mimedir_valarm_new (MIMEDIR_VALARM_PROCEDURE);
		/* FIXME: anchor */
		priv->alarms = g_list_prepend (priv->alarms, alarm);

		mimedir_attribute_free_structured_text_list (list);
	}

	/* Mail Alarm (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "MALARM")) {
		MIMEDirVAlarm *alarm;
		GSList *list;

		list = mimedir_attribute_get_value_structured_text (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		alarm = mimedir_valarm_new (MIMEDIR_VALARM_EMAIL);
		/* FIXME: anchor */
		priv->alarms = g_list_prepend (priv->alarms, alarm);

		mimedir_attribute_free_structured_text_list (list);
	}

	/* (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "X-PILOTID")) {
		/* FIXME */
	}

	/* (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "X-PILOTSTAT")) {
		/* FIXME */
	}

	/* (GNOME-PIM 1) */

	else if (!g_ascii_strcasecmp (name, "X-PILOT-NOTIME")) {
		gint i;

		i = mimedir_attribute_get_value_int (attr, &err);
		if (err) {
			g_propagate_error (error, err);
			return FALSE;
		}

		priv->allday = i == 0 ? FALSE : TRUE;
	}

	/* Custom Attributes */

	else if (!g_ascii_strncasecmp (name, "X-", 2))
		g_printerr (_("The profile contains the unsupported custom attribute %s.\n"), name);

	else
		g_printerr (_("The profile contains the unknown attribute %s.\n"), name);

	return TRUE;
}

/* Create a new FREEBUSY attribute */
static MIMEDirAttribute *
mimedir_vcomponent_new_freebusy_attribute (MIMEDirVFreeBusyType fbtype, GSList *slist)
{
	MIMEDirAttribute *attr;

	attr = mimedir_attribute_new_with_name ("FREEBUSY");
	mimedir_attribute_set_value_text_list (attr, slist);
	if (fbtype != MIMEDIR_VFREEBUSY_BUSY) {
		const gchar *s;

		switch (fbtype) {
		case MIMEDIR_VFREEBUSY_FREE:
			s = "FREE";
			break;
		case MIMEDIR_VFREEBUSY_BUSY:
			/* dummy */
			break;
		case MIMEDIR_VFREEBUSY_BUSY_UNAVAILABLE:
			s = "BUSY-UNAVAILABLE";
			break;
		case MIMEDIR_VFREEBUSY_BUSY_TENTATIVE:
			s = "BUSY-TENTATIVE";
			break;
		default:
			g_return_val_if_reached (NULL);
		}
		mimedir_attribute_append_parameter_simple (attr, "FBTYPE", s);
	}

	return attr;
}

/* Write FREEBUSY information to the supplied profile */
static void
mimedir_vcomponent_write_freebusy (MIMEDirVComponent *vcomponent, MIMEDirProfile *profile)
{
	MIMEDirVComponentPriv *priv;
	GList *node;
	GSList *slist = NULL;
	MIMEDirVFreeBusyType oldfbtype = -1;

	priv = vcomponent->priv;

	for (node = priv->freebusy; node != NULL; node = g_list_next (node)) {
		MIMEDirPeriod *period;
		MIMEDirVFreeBusyType fbtype;
		gchar *p;

		g_assert (MIMEDIR_IS_PERIOD (node->data));
		period = MIMEDIR_PERIOD (node->data);

		g_object_get (G_OBJECT (period),
			      "fbtype", &fbtype,
			      NULL);

		if (fbtype != oldfbtype) {
			if (slist) {
				MIMEDirAttribute *attr;

				slist = g_slist_reverse (slist);

				attr = mimedir_vcomponent_new_freebusy_attribute (oldfbtype, slist);
				mimedir_profile_append_attribute (profile, attr);
				g_object_unref (G_OBJECT (attr));

				mimedir_utils_free_string_slist (slist);
				slist = NULL;
			}
			oldfbtype = fbtype;
		}

		p = mimedir_period_get_as_mimedir (period);
		slist = g_slist_prepend (slist, p);
	}

	if (slist) {
		MIMEDirAttribute *attr;

		slist = g_slist_reverse (slist);

		attr = mimedir_vcomponent_new_freebusy_attribute (oldfbtype, slist);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));

		mimedir_utils_free_string_slist (slist);
		slist = NULL;
	}
}

/*
 * Public Methods
 */

/**
 * mimedir_vcomponent_read_from_profile:
 * @vcomponent: a vComponent object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vComponent object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcomponent_read_from_profile (MIMEDirVComponent *vcomponent, MIMEDirProfile *profile, GError **error)
{
	MIMEDirVComponentPriv *priv;
	GSList *attrs, *components;

	g_return_val_if_fail (vcomponent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	mimedir_vcomponent_reset (vcomponent);
	priv = vcomponent->priv;

	attrs = mimedir_profile_get_attribute_list (profile);

	for (; attrs != NULL; attrs = g_slist_next (attrs)) {
		MIMEDirAttribute *attr;

		attr = MIMEDIR_ATTRIBUTE (attrs->data);

		if (!mimedir_vcomponent_parse_attribute (vcomponent, attr, error))
			return FALSE;
	}

	/* Read sub-profiles (vAlarms) */
	components = mimedir_profile_get_subprofiles (profile);

	for (; components != NULL; components = g_slist_next (components)) {
		MIMEDirProfile *profile;
		gchar *name;

		profile = MIMEDIR_PROFILE (components->data);

		g_object_get (G_OBJECT (profile), "name", &name, NULL);

		if (!g_ascii_strcasecmp (name, "VALARM")) {
			MIMEDirVAlarm *alarm;

			alarm = mimedir_valarm_new_from_profile (profile, error);
			if (!alarm) {
				g_free (name);
				return FALSE;
			}

			priv->alarms = g_list_prepend(priv->alarms, alarm);
		}

		g_free (name);
	}

	/* Validity checks */

	/* FIXME: dtend must be >= dtstart */

	g_signal_emit (G_OBJECT (vcomponent), mimedir_vcomponent_signals[SIGNAL_CHANGED], 0);
	g_signal_emit (G_OBJECT (vcomponent), mimedir_vcomponent_signals[SIGNAL_ALARMS_CHANGED], 0);

	return TRUE;
}

/**
 * mimedir_vcomponent_write_to_profile:
 * @vcomponent: a vcomponent
 * @profile_name: the new profile type
 *
 * Saves the vcomponent object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vcomponent_write_to_profile (MIMEDirVComponent *vcomponent, const gchar *profile_name)
{
	MIMEDirVComponentPriv *priv;
	MIMEDirProfile *profile;
	MIMEDirAttribute *attr;
	GList *l;

	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	priv = vcomponent->priv;

#if 0 /* FIXME */
	if (priv->changed || priv->revision.year == 0)
		mimedir_vcomponent_update_revision (vcomponent);
#endif

	profile = mimedir_profile_new (profile_name);

	/* FIXME: required: uid */
	/* FIXME: sequence */

	/*
	 * Descriptive Properties (RFC 2445, Section 4.8.1)
	 */

	/* Attachment (4.8.1.1) */

	for (l = priv->attachments; l != NULL; l = g_list_next (l)) {
		MIMEDirAttachment *attachment;

		g_assert (l->data != NULL && MIMEDIR_IS_ATTACHMENT (l->data));
		attachment = MIMEDIR_ATTACHMENT (l->data);

		attr = mimedir_attachment_write_to_attribute (attachment);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Categories (4.8.1.2) */

	if (priv->categories) {
		GSList *gsl = NULL;
		GList *gl;

		for (gl = priv->categories; gl != NULL; gl = g_list_next (gl))
			gsl = g_slist_prepend (gsl, gl->data);
		gsl = g_slist_reverse (gsl);

		attr = mimedir_attribute_new_with_name ("CATEGORIES");
		mimedir_attribute_set_value_text_list (attr, gsl);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));

		g_slist_free (gsl);
	}

	/* Classification (4.8.1.3) */

	attr = NULL;

	switch (priv->md_class) {
	case MIMEDIR_CLASS_PUBLIC:
		break; /* dummy */
	case MIMEDIR_CLASS_PRIVATE:
		attr = mimedir_attribute_new_with_name ("CLASS");
		mimedir_attribute_set_value (attr, "PRIVATE");
		break;
	case MIMEDIR_CLASS_CONFIDENTIAL:
		attr = mimedir_attribute_new_with_name ("CLASS");
		mimedir_attribute_set_value (attr, "CONFIDENTIAL");
		break;
	case MIMEDIR_CLASS_CUSTOM:
		g_assert (priv->md_class_str != NULL);
		attr = mimedir_attribute_new_with_name ("CLASS");
		mimedir_attribute_set_value (attr, priv->md_class_str);
		break;
	default:
		g_assert ("wrong classification");
	}

	if (attr) {
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Comment (4.8.1.4) */

	for (l = priv->comment; l != NULL; l = g_list_next (l)) {
		gchar *comment;

		g_assert (l->data);
		comment = (gchar *) l->data;

		attr = mimedir_attribute_new_with_name ("COMMENT");
		mimedir_attribute_set_value_text (attr, comment);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Description (4.8.1.5) */

	if (priv->description) {
		attr = mimedir_attribute_new_with_name ("DESCRIPTION");
		mimedir_attribute_set_value_text (attr, priv->description);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Geographic Position (4.8.1.6) */

	if (priv->latitude != 0.0 || priv->longitude != 0.0) {
		GSList *list = NULL;
		gdouble lat, lon;

		lat = priv->latitude;
		lon = priv->longitude;

		list = g_slist_append (list, &lat);
		list = g_slist_append (list, &lon);

		attr = mimedir_attribute_new_with_name ("GEO");
		mimedir_attribute_set_value_float_list (attr, list);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));

		g_slist_free (list);
	}

	/* Location (4.8.1.7) */

	if (priv->location) {
		attr = mimedir_attribute_new_with_name ("LOCATION");
		mimedir_attribute_set_value_text (attr, priv->location);
		if (priv->location_alt)
			mimedir_attribute_append_parameter_simple (attr, "altrep", priv->location_alt);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Percent Complete (4.8.1.8) */

	if (priv->percent != -1) {
		attr = mimedir_attribute_new_with_name ("PERCENT-COMPLETE");
		mimedir_attribute_set_value_int (attr, priv->percent);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Priority (4.8.1.9) */

	if (priv->priority > 0) {
		attr = mimedir_attribute_new_with_name ("PRIORITY");
		mimedir_attribute_set_value_int (attr, priv->priority);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Resources (4.8.1.10) */

	if (priv->resources) {
		GSList *list = NULL;
		GList *node;

		for (node = priv->resources; node != NULL; node = g_list_next (node))
			list = g_slist_prepend (list, node->data);
		list = g_slist_reverse (list);

		attr = mimedir_attribute_new_with_name ("RESOURCES");
		mimedir_attribute_set_value_text_list (attr, list);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Status (4.8.1.11) */

	if (priv->status != MIMEDIR_STATUS_UNKNOWN) {
		const gchar *s;

		switch (priv->status) {
		case MIMEDIR_STATUS_CANCELLED:
			s = "CANCELLED";
			break;
		case MIMEDIR_STATUS_TENTATIVE:
			s = "TENTATIVE";
			break;
		case MIMEDIR_STATUS_CONFIRMED:
			s = "CONFIRMED";
			break;
		case MIMEDIR_STATUS_NEEDS_ACTION:
			s = "NEEDS-ACTION";
			break;
		case MIMEDIR_STATUS_COMPLETED:
			s = "COMPLETED";
			break;
		case MIMEDIR_STATUS_IN_PROCESS:
			s = "IN-PROCESS";
			break;
		case MIMEDIR_STATUS_DRAFT:
			s = "DRAFT";
			break;
		case MIMEDIR_STATUS_FINAL:
			s = "FINAL";
			break;
		default:
			g_return_val_if_reached (NULL);
		}

		attr = mimedir_attribute_new_with_name ("STATUS");
		mimedir_attribute_set_value (attr, s);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Summary (4.8.1.12) */

	if (priv->summary) {
		attr = mimedir_attribute_new_with_name ("SUMMARY");
		mimedir_attribute_set_value_text (attr, priv->summary);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Date and Time Properties (RFC 2445, Section 4.8.2)
	 */

	/* Date/Time Completed (4.8.2.1) */

	if (priv->dtcompleted && mimedir_datetime_is_valid (priv->dtcompleted)) {
		attr = mimedir_attribute_new_with_name ("COMPLETED");
		mimedir_attribute_set_value_datetime (attr, priv->dtcompleted);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Date/Time End (4.8.2.2) */

	if (priv->dtend && mimedir_datetime_is_valid (priv->dtend)) {
		attr = mimedir_attribute_new_with_name ("DTEND");
		mimedir_attribute_set_value_datetime (attr, priv->dtend);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Date/Time Due (4.8.2.3) */

	if (priv->due && mimedir_datetime_is_valid (priv->due)) {
		attr = mimedir_attribute_new_with_name ("DUE");
		mimedir_attribute_set_value_datetime (attr, priv->due);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Date/Time Start (4.8.2.4) */

	if (priv->dtstart && mimedir_datetime_is_valid (priv->dtstart)) {
		attr = mimedir_attribute_new_with_name ("DTSTART");
		mimedir_attribute_set_value_datetime (attr, priv->dtstart);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Duration (4.8.2.5) */

	if (priv->duration > 0) {
		attr = mimedir_attribute_new_with_name ("DURATION");
		mimedir_vcomponent_set_value_duration (attr, priv->duration);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Free/Busy Time (4.8.2.6) */

	if (priv->freebusy)
		mimedir_vcomponent_write_freebusy (vcomponent, profile);

	/* Time Transparency (4.8.2.7) */

	if (!priv->opaque) {
		attr = mimedir_attribute_new_with_name ("TRANSP");
		mimedir_attribute_set_value (attr, "TRANSPARENT");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Time Zone Properties (RFC 2445, Section 4.8.3)
	 */

	/* Time Zone Identifier (4.8.3.1) */

	if (priv->tzid) {
		attr = mimedir_attribute_new_with_name ("TZID");
		mimedir_attribute_set_value (attr, priv->tzid);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Time Zone Name (4.8.3.2) */

	for (l = priv->tznames; l != NULL; l = g_list_next (l)) {
		attr = mimedir_attribute_new_with_name ("TZNAME");
		mimedir_attribute_set_value_text (attr, (gchar *) l->data);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Time Zone Offset From (4.8.3.3) */

	if (MIMEDIR_IS_VTIMEZONE (vcomponent)) {
		attr = mimedir_attribute_new_with_name ("TZOFFSETFROM");
		mimedir_vcomponent_set_value_utcoffset (attr, priv->tzofffrom);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Time Zone Offset To (4.8.3.4) */

	if (MIMEDIR_IS_VTIMEZONE (vcomponent)) {
		attr = mimedir_attribute_new_with_name ("TZOFFSETTO");
		mimedir_vcomponent_set_value_utcoffset (attr, priv->tzoffto);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Time Zone URL (4.8.3.5) */

	if (priv->tzurl) {
		attr = mimedir_attribute_new_with_name ("TZURL");
		mimedir_attribute_set_value_uri (attr, priv->tzurl);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Relationship Properties (RFC 2445, Section 4.8.4)
	 */

	/* Attendee (4.8.4.1) */

	for (l = priv->attendees; l != NULL; l = g_list_next (l)) {
		MIMEDirAttendee *att;

		att = l->data;
		g_assert (att != NULL && MIMEDIR_IS_ATTENDEE (att));

		attr = mimedir_attendee_write_to_attribute (att);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Contact (4.8.4.2) */

	if (priv->contact) {
		attr = mimedir_attribute_new_with_name ("CONTACT");
		mimedir_attribute_set_value_text (attr, priv->contact);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Organizer (4.8.4.3) */

	/* FIXME */

	/* Recurrence ID (4.8.4.4) */

	/* FIXME */

	/* Related To (4.8.4.5) */

	/* FIXME */

	/* Uniform Resource Locator (4.8.4.6) */

	/* FIXME */

	/* Unique Identifier (4.8.4.7) */

	if (priv->uid) {
		attr = mimedir_attribute_new_with_name ("UID");
		mimedir_attribute_set_value_text (attr, priv->uid);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Recurrence Properties (RFC 2445, Section 4.8.5)
	 */

	/* Exception Date/Times (4.8.5.1) */

	/* FIXME */

	/* Exception Rule (4.8.5.2) */

	/* FIXME */

	/* Recurrence Date/Times (4.8.5.3) */

	/* FIXME */

	/* Recurrence Rule (4.8.5.4) */

	if (priv->recurrence) {
		gchar *rrule_s;

		attr = mimedir_attribute_new_with_name ("RRULE");
		rrule_s = mimedir_recurrence_write_to_string (priv->recurrence);
		mimedir_attribute_set_value_text_unescaped (attr, rrule_s);
		g_free (rrule_s);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Alarm Properties (RFC 2445, Section 4.8.6)
	 */

	/* Action (4.8.6.1) */

	if (priv->action) {
		attr = mimedir_attribute_new_with_name ("ACTION");
		mimedir_attribute_set_value_text (attr, priv->action);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Repeat Count (4.8.6.2) */

	if (priv->repeat > 0) {
		attr = mimedir_attribute_new_with_name ("REPEAT");
		mimedir_attribute_set_value_int (attr, (gint) priv->repeat);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Trigger (4.8.6.3) */

	if (!g_ascii_strcasecmp (profile_name, "VALARM")) {
		attr = mimedir_attribute_new_with_name ("TRIGGER");
		if (priv->trigger_dt) {
			mimedir_attribute_append_parameter_simple (attr, "VALUE", "DATE-TIME");
			mimedir_attribute_set_value_datetime (attr, priv->trigger_dt);
		} else {
			mimedir_vcomponent_set_value_duration (attr, priv->trigger);
			if (priv->trigger_end)
				mimedir_attribute_append_parameter_simple (attr, "RELATED", "END");
		}
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Change Management Properties (RFC 2445, Section 4.8.7)
	 */

	/* Date/Time Created (4.8.7.1) */

	if (priv->created) {
		attr = mimedir_attribute_new_with_name ("CREATED");
		mimedir_attribute_set_value_datetime (attr, priv->created);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Date/Time Stamp (4.8.7.2) */

	if (priv->dtstamp) {
		attr = mimedir_attribute_new_with_name ("DTSTAMP");
		mimedir_attribute_set_value_datetime (attr, priv->dtstamp);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Last Modified (4.8.7.3) */

	if (priv->last_modified) {
		attr = mimedir_attribute_new_with_name ("LAST-MODIFIED");
		mimedir_attribute_set_value_datetime (attr, priv->last_modified);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Sequence Number (4.8.7.4) */

	if (priv->sequence >= 0) {
		attr = mimedir_attribute_new_with_name ("SEQUENCE");
		mimedir_attribute_set_value_int (attr, priv->sequence);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Miscellaneous Properties (RFC 2445, Section 4.8.8)
	 */

	/* Non-Standard Properties (4.8.8.1) handled below */

	/* Request Status (4.8.8.2) */

	/* FIXME */

	/*
	 * Alarms
	 */

	for (l = priv->alarms; l != NULL; l = g_list_next (l)) {
		MIMEDirVAlarm *alarm;
		MIMEDirProfile *prof;

		g_assert (l->data != NULL && MIMEDIR_IS_VALARM (l->data));
		alarm = MIMEDIR_VALARM (l->data);

		prof = mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (alarm), "VALARM");
		mimedir_profile_append_subprofile (profile, prof);
		g_object_unref (G_OBJECT (prof));
	}

	/*
	 * Finish
	 */

	priv->changed = FALSE;

	return profile;
}

static void
mimedir_vcomponent_add_category_to_string (gpointer data, gpointer user_data)
{
	GString     *string;
	const gchar *text;

	g_return_if_fail (data != NULL);
	g_return_if_fail (user_data != NULL);

	text   = (const gchar *) data;
	string = (GString *)     user_data;

	if (string->str[0] != '\0')
		g_string_append (string, ", ");
	g_string_append (string, text);
}

/**
 * mimedir_vcomponent_set_attachment_list:
 * @vcomponent: a #MIMEDirVComponent object
 * @list: #GList of #MIMEDirAttachment objects
 *
 * Sets the list of attachments of @vcomponent to @list, freeing the old list.
 **/
void
mimedir_vcomponent_set_attachment_list (MIMEDirVComponent *vcomponent, GList *list)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	mimedir_utils_free_object_list (vcomponent->priv->attachments);
	vcomponent->priv->attachments = mimedir_utils_copy_object_list (list);
}

/**
 * mimedir_vcomponent_get_attachment_list:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns the list of attachments of @vcomponent. This list is to be
 * considered read-only and must not be altered in any way.
 *
 * Return value: read-only #GList of #MIMEDirAttachment objects
 **/
GList *
mimedir_vcomponent_get_attachment_list (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	return vcomponent->priv->attachments;
}

/**
 * mimedir_vcomponent_get_categories_as_string:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns the categories of this object as a human-readable,
 * comma-separated string. The returned string should be freed
 * with g_free().
 *
 * Return value: a human-readable string
 **/
gchar *
mimedir_vcomponent_get_categories_as_string	(MIMEDirVComponent *vcomponent)
{
	GString *string;

	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	string = g_string_new ("");

	g_list_foreach (vcomponent->priv->categories,
			mimedir_vcomponent_add_category_to_string,
			string);

        return g_string_free (string, FALSE);
}

/**
 * mimedir_vcomponent_set_classification:
 * @vcomponent: a #MIMEDirVComponent object
 * @classification: the new security classification
 * @klass: textual security classficiation
 *
 * Sets the new security classification to @classification. If and only if
 * the classification is %MIMEDIR_CLASS_CUSTOM @klass must be pointing to
 * a string to use as security classification. Be sure you know what you're
 * doing when using %MIMEDIR_CLASS_CUSTOM!
 **/
void
mimedir_vcomponent_set_classification (MIMEDirVComponent *vcomponent, MIMEDirClassification classification, const gchar *klass)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));
	g_return_if_fail (classification >= MIMEDIR_CLASS_PUBLIC && classification <= MIMEDIR_CLASS_CUSTOM);
	g_return_if_fail (classification == MIMEDIR_CLASS_CUSTOM || klass == NULL);
	g_return_if_fail (classification != MIMEDIR_CLASS_CUSTOM || klass != NULL);

	vcomponent->priv->md_class = classification;
	g_free (vcomponent->priv->md_class_str);
	vcomponent->priv->md_class_str = g_strdup (klass);
}

/**
 * mimedir_vcomponent_get_classification:
 * @vcomponent: a #MIMEDirVComponent object
 * @klass: pointer to a string storage location or %NULL
 *
 * Returns the component's security classification. If the class ist
 * %MIMEDIR_CLASS_CUSTOM and @klass is not %NULL, @klass will be filled
 * to point to a string that signifies the security classification.
 *
 * Return value: the security classification
 **/
MIMEDirClassification
mimedir_vcomponent_get_classification (MIMEDirVComponent *vcomponent, const gchar **klass)
{
	g_return_val_if_fail (vcomponent != NULL, MIMEDIR_CLASS_PUBLIC);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), MIMEDIR_CLASS_PUBLIC);

	if (vcomponent->priv->md_class == MIMEDIR_CLASS_CUSTOM) {
		g_assert (vcomponent->priv->md_class_str != NULL);

		if (klass)
			*klass = g_strdup (vcomponent->priv->md_class_str);
	}

	return vcomponent->priv->md_class;
}

/**
 * mimedir_vcomponent_set_geopos:
 * @vcomponent: a #MIMEDirVComponent object
 * @latitude: latitude from -90 to +90 degrees
 * @longitude: longitude from -180 to + 180 degrees
 *
 * Sets this component's geographic position to @latitude/@longitude.
 **/
void
mimedir_vcomponent_set_geopos (MIMEDirVComponent *vcomponent, gdouble latitude, gdouble longitude)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));
	g_return_if_fail (latitude >= -90.0 && latitude <= 90.0);
	g_return_if_fail (longitude >= -180.0 && longitude <= 180.0);

	vcomponent->priv->latitude  = latitude;
	vcomponent->priv->longitude = longitude;
}

/**
 * mimedir_vcomponent_get_geopos:
 * @vcomponent: a #MIMEDirVComponent object
 * @latitude: pointer to a #gdouble value or %NULL
 * @longitude: pointer to a #gdouble value or %NULL
 *
 * Sets @latitude and @longitude to the component's geographical position.
 **/
void
mimedir_vcomponent_get_geopos (MIMEDirVComponent *vcomponent, gdouble *latitude, gdouble *longitude)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	if (latitude)
		*latitude  = vcomponent->priv->latitude;
	if (longitude)
		*longitude = vcomponent->priv->longitude;
}

/**
 * mimedir_vcomponent_set_location:
 * @vcomponent: a #MIMEDirVComponent object
 * @location: location specification or %NULL
 * @uri: optional alternate URI or %NULL
 *
 * Sets the venue, such as a conference or meeting room. @uri, if not %NULL,
 * points to an alternate representation such as an web site or an LDAP
 * entry.
 *
 * If @location is %NULL the property will be unset. In this case, @uri
 * must be %NULL as well.
 **/
void
mimedir_vcomponent_set_location (MIMEDirVComponent *vcomponent, const gchar *location, const gchar *uri)
{
	MIMEDirVComponentPriv *priv;

	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));
	g_return_if_fail (location != NULL || uri == NULL);

	priv = vcomponent->priv;
	
	g_free (priv->location);
	g_free (priv->location_alt);
	priv->location     = g_strdup (location);
	priv->location_alt = uri ? g_strdup (uri) : NULL;
}

/**
 * mimedir_vcomponent_get_location:
 * @vcomponent: a #MIMEDirVComponent object
 * @uri: uri storage pointer or %NULL
 *
 * Retrieves the venue, such as a conference or meeting room. If this property
 * is not set, %NULL will be returned.
 *
 * If @uri is not %NULL, it will be set to an URI to an alternate
 * representation (if one was set). If no alternate representation URI
 * was given, @uri will be set to %NULL.
 *
 * Return value: location string or %NULL
 **/
const gchar *
mimedir_vcomponent_get_location (MIMEDirVComponent *vcomponent, const gchar **uri)
{
	MIMEDirVComponentPriv *priv;

	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	priv = vcomponent->priv;

	if (uri)
		*uri = priv->location_alt;

	return priv->location;
}

/**
 * mimedir_vcomponent_set_opaque:
 * @vcomponent: a #MIMEDirVComponent object
 * @opaque: %TRUE or %FALSE
 *
 * Sets whether an event is to be considered opaque or transparent. A
 * transparent event does not consume time on the calendar.
 **/
void
mimedir_vcomponent_set_opaque (MIMEDirVComponent *vcomponent, gboolean opaque)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	vcomponent->priv->opaque = opaque ? TRUE : FALSE;
}

/**
 * mimedir_vcomponent_get_opaque:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns whether an event is opaque or transparent. Transparent events
 * do not consume time on the calendar.
 *
 * Return value: a boolean value
 **/
gboolean
mimedir_vcomponent_get_opaque (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), FALSE);

	return vcomponent->priv->opaque;
}

/**
 * mimedir_vcomponent_set_allday:
 * @vcomponent: a #MIMEDirVComponent object
 * @allday: %TRUE or %FALSE
 *
 * Set the allday flag to @allday.
 **/
void
mimedir_vcomponent_set_allday (MIMEDirVComponent *vcomponent, gboolean allday)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	vcomponent->priv->allday = allday;
}

/**
 * mimedir_vcomponent_get_allday:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Return %TRUE if the event lasts a whole day, %FALSE otherwise.
 *
 * Return value: %TRUE or %FALSE
 **/
gboolean
mimedir_vcomponent_get_allday (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), FALSE);

	if (vcomponent->priv->allday)
		return TRUE;

	return FALSE; /* FIXME: check date/time boundaries */
}

/**
 * mimedir_vcomponent_set_alarm_list:
 * @vcomponent: a #MIMEDirVComponent object
 * @list: a #GList pointer
 *
 * Sets the list of alarms of this component.
 **/
void
mimedir_vcomponent_set_alarm_list (MIMEDirVComponent *vcomponent, const GList *list)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	mimedir_utils_free_object_list (vcomponent->priv->alarms);
	vcomponent->priv->alarms = mimedir_utils_copy_object_list (list);

	g_signal_emit (G_OBJECT (vcomponent), mimedir_vcomponent_signals[SIGNAL_ALARMS_CHANGED], 0);
}

/**
 * mimedir_vcomponent_get_alarm_list:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns all alarms of this component. The list must not be modified
 * by the caller. The #MIMEDirVAlarm objects in this list must be
 * g_object_ref()'ed when they are to be used by the caller.
 *
 * Return value: a #GList containing #MIMEDirVAlarm objects
 **/
const GList *
mimedir_vcomponent_get_alarm_list (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	return vcomponent->priv->alarms;
}

/**
 * mimedir_vcomponent_set_attendee_list:
 * @vcomponent: a #MIMEDirVComponent object
 * @list: a #GList pointer
 *
 * Sets the list of attendees for this component.
 **/
void
mimedir_vcomponent_set_attendee_list (MIMEDirVComponent *vcomponent, const GList *list)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));

	mimedir_utils_free_object_list (vcomponent->priv->attendees);
	vcomponent->priv->attendees = mimedir_utils_copy_object_list (list);

	g_signal_emit (G_OBJECT (vcomponent), mimedir_vcomponent_signals[SIGNAL_CHANGED], 0);
}

/**
 * mimedir_vcomponent_get_attendee_list:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns the list of attendees to this particular component.
 *
 * Return value: a #GList containing #MIMEDirAttendee objects
 **/
const GList *
mimedir_vcomponent_get_attendee_list (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	return vcomponent->priv->attendees;
}

/**
 * mimedir_vcomponent_does_recur:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Return %TRUE if the component has an associated recurrence object.
 *
 * Return value: %TRUE or %FALSE
 **/
gboolean
mimedir_vcomponent_does_recur (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), FALSE);

	return vcomponent->priv->recurrence != NULL;
}

/**
 * mimedir_vcomponent_set_recurrence:
 * @vcomponent: a #MIMEDirVComponent object
 * @recurrence: a #MIMEDirRecurrence object or %NULL
 *
 * Sets the recurrence information for @vcomponent. If @recurrence is
 * %NULL, this component does not recur.
 **/
void
mimedir_vcomponent_set_recurrence (MIMEDirVComponent *vcomponent, MIMEDirRecurrence *recurrence)
{
	g_return_if_fail (vcomponent != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent));
	g_return_if_fail (recurrence == NULL || MIMEDIR_IS_RECURRENCE (recurrence));

	if (vcomponent->priv->recurrence) {
		g_object_unref (G_OBJECT (vcomponent->priv->recurrence));
		vcomponent->priv->recurrence = NULL;
	}

	if (recurrence) {
		g_object_ref (G_OBJECT (recurrence));
		vcomponent->priv->recurrence = recurrence;
	}
}

/**
 * mimedir_vcomponent_get_recurrence:
 * @vcomponent: a #MIMEDirVComponent object
 *
 * Returns the recurrence object associated with this component or %NULL
 * if there is no recurrence object.
 *
 * Return value: a #MIMEDirRecurrence object or %NULL
 **/
MIMEDirRecurrence *
mimedir_vcomponent_get_recurrence (MIMEDirVComponent *vcomponent)
{
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);

	return vcomponent->priv->recurrence;
}

/**
 * mimedir_vcomponent_get_next_occurence:
 * @vcomponent: a #MIMEDirVComponent object
 * @after: a #MIMEDirDateTime object
 *
 * Returns the first occurence of this component after or at the date
 * specified by the @after argument. If @after is %NULL, the very first
 * occurence is returned. If there is no occurence after the specified date,
 * %NULL is returned.
 *
 * Return value: a #MIMEDirDateTime object or %NULL
 **/
MIMEDirDateTime *
mimedir_vcomponent_get_next_occurence (MIMEDirVComponent *vcomponent, MIMEDirDateTime *after)
{
	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);
	g_return_val_if_fail (after != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (after), NULL);

	/* FIXME */

	return NULL;
}

/**
 * mimedir_vcomponent_get_occurences_between:
 * @vcomponent: a #MIMEDirVComponent object
 * @start: start time
 * @end: end time, may be %NULL
 *
 * Return a list of #MIMEDirDateTime objects that specify the start and
 * end dates of all occurences of @vcomponent that overlap with the time
 * interval between @start and @end. If @end is %NULL, all occurences that
 * intersect with @start are returned.
 *
 * Every second element in the list may be %NULL in which case there is
 * no end time for the occurence.
 *
 * The returned list should be freed with mimedir_vcomponent_free_occurences().
 *
 * Return value: a #GList containing #MIMEDirDateTime objects
 **/
GList *
mimedir_vcomponent_get_occurences_between (MIMEDirVComponent *vcomponent, MIMEDirDateTime *start, MIMEDirDateTime *end)
{
	MIMEDirDateTime *dtstart, *dtend;
	GDate start_gd, end_gd, dtstart_gd, dtend_gd;
	GList *list = NULL;

	g_return_val_if_fail (vcomponent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCOMPONENT (vcomponent), NULL);
	g_return_val_if_fail (start != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_DATETIME (start), NULL);
	g_return_val_if_fail (end == NULL || MIMEDIR_IS_DATETIME (end), NULL);

	g_object_get (G_OBJECT (vcomponent),
		      "dtstart", &dtstart,
		      "dtend",   &dtend,
		      NULL);

	mimedir_datetime_get_gdate (dtstart, &dtstart_gd);
	mimedir_datetime_get_gdate (dtend, &dtend_gd);
	mimedir_datetime_get_gdate (start, &start_gd);
	mimedir_datetime_get_gdate (end, &end_gd);

	if (g_date_compare (&dtstart_gd, &dtend_gd) > 0)
		return NULL;

	/* FIXME: handle recurrence */

	if (g_date_compare (&dtstart_gd, &end_gd) > 0 ||
	    g_date_compare (&dtend_gd, &start_gd) < 0)
		return NULL;

	g_object_ref (G_OBJECT (dtstart));
	g_object_ref (G_OBJECT (dtend));

	list = g_list_prepend (list, dtstart);
	list = g_list_prepend (list, dtend);

	list = g_list_reverse (list);

	return list;
}

/**
 * mimedir_vcomponent_free_occurences:
 * @list: a #GList as returned by mimedir_vcomponent_get_occurences_between()
 *
 * Frees a #GList as returned by mimedir_vcomponent_get_occurences_between().
 **/
void
mimedir_vcomponent_free_occurences (GList *list)
{
	GList *l;

	if (!list)
		return;

	for (l = list; l != NULL; l = g_list_next (l)) {
		if (l->data) {
			g_assert (MIMEDIR_IS_DATETIME (l->data));
			g_object_unref (G_OBJECT (l->data));
		}
	}

	g_list_free (list);
}
