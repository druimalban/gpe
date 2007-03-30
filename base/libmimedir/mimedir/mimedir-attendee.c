/* RFC 2445 Attendee Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attendee.c 176 2005-02-26 22:46:04Z srittau $
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

#include "mimedir-attendee.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_attendee_class_init	(MIMEDirAttendeeClass	*klass);
static void	 mimedir_attendee_init		(MIMEDirAttendee	*attendee);
static void	 mimedir_attendee_dispose	(GObject		*object);


struct _MIMEDirAttendeePriv {
	gchar	*cn;			/* Common Name */
	gchar	*role;			/* Role */
	gchar	*partstat;		/* Participation Status */
	gchar	*rsvp;			/* Reply Requested */
	gchar	*cutype;		/* Calendar User Type */
	gchar	*member;		/* Groups the Attendee Belongs to */
	gchar	*delegated_to;		/* Delegated To */
	gchar	*delegated_from;	/* Delegated From */
	gchar	*sent_by;		/* Sent By */
	gchar	*dir;			/* Directory URI */
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_attendee_get_type (void)
{
	static GType mimedir_attendee_type = 0;

	if (!mimedir_attendee_type) {
		static const GTypeInfo mimedir_attendee_info = {
			sizeof (MIMEDirAttendeeClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_attendee_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirAttendee),
			4,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_attendee_init,
		};

		mimedir_attendee_type = g_type_register_static (G_TYPE_OBJECT,
								"MIMEDirAttendee",
								&mimedir_attendee_info,
								0);
	}

	return mimedir_attendee_type;
}


static void
mimedir_attendee_class_init (MIMEDirAttendeeClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTENDEE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = mimedir_attendee_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_attendee_init (MIMEDirAttendee *attendee)
{
	MIMEDirAttendeePriv *priv;

	g_return_if_fail (attendee != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTENDEE (attendee));

	priv = g_new0 (MIMEDirAttendeePriv, 1);
	attendee->priv = priv;
}


static void
mimedir_attendee_dispose (GObject *object)
{
	MIMEDirAttendee *attendee;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTENDEE (object));

	attendee = MIMEDIR_ATTENDEE (object);

	if (attendee->priv) {
		g_free (attendee->priv->cn);
		g_free (attendee->priv->role);
		g_free (attendee->priv->partstat);
		g_free (attendee->priv->rsvp);
		g_free (attendee->priv->cutype);
		g_free (attendee->priv->member);
		g_free (attendee->priv->delegated_to);
		g_free (attendee->priv->delegated_from);
		g_free (attendee->priv->sent_by);
		g_free (attendee->priv->dir);

		g_free (attendee->priv);
		attendee->priv = NULL;
	}

	g_free (attendee->uri);
	attendee->uri = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_attendee_new:
 *
 * Creates a new attendee object.
 *
 * Return value: a new attendee object
 **/
MIMEDirAttendee *
mimedir_attendee_new (void)
{
	MIMEDirAttendee *att;

	att = g_object_new (MIMEDIR_TYPE_ATTENDEE, NULL);

	return att;
}

/**
 * mimedir_attendee_new_from_attribute:
 * @attribute: a #MIMEDirAttribute object
 * @error: error storage location or %NULL
 *
 * Creates a new attendee object and initializes it with values read
 * from the supplied attribute object.
 *
 * Return value: a new #MIMEDirAttendee object or %NULL on error
 **/
MIMEDirAttendee	*
mimedir_attendee_new_from_attribute (MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirAttendee *att;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	att = g_object_new (MIMEDIR_TYPE_ATTENDEE, NULL);

	if (!mimedir_attendee_read_from_attribute (att, attribute, error)) {
		g_object_unref (G_OBJECT (att));
		return NULL;
	}

	return att;
}

/**
 * mimedir_attendee_read_from_attribute:
 * @attendee: a #MIMEDirAttendee object
 * @attribute: a #MIMEDirAttribute object
 * @error: error storage location or %NULL
 *
 * Initializes @attendee with data read from @attribute.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_attendee_read_from_attribute (MIMEDirAttendee *attendee, MIMEDirAttribute *attribute, GError **error)
{
	const gchar *attribute_name;

	g_return_val_if_fail (attendee != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTENDEE (attendee), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	attribute_name = mimedir_attribute_get_name (attribute);

	g_return_val_if_fail (!g_ascii_strcasecmp (attribute_name, "ATTENDEE"), FALSE);

	g_free (attendee->uri);
	attendee->uri = g_strdup (mimedir_attribute_get_value (attribute));

	attendee->priv->cn             = g_strdup (mimedir_attribute_get_parameter_value (attribute, "CN"));
	attendee->priv->role           = g_strdup (mimedir_attribute_get_parameter_value (attribute, "ROLE"));
	attendee->priv->partstat       = g_strdup (mimedir_attribute_get_parameter_value (attribute, "PARTSTAT"));
	attendee->priv->rsvp           = g_strdup (mimedir_attribute_get_parameter_value (attribute, "RSVP"));
	attendee->priv->cutype         = g_strdup (mimedir_attribute_get_parameter_value (attribute, "CUTYPE"));
	attendee->priv->member         = g_strdup (mimedir_attribute_get_parameter_value (attribute, "MEMBER"));
	attendee->priv->delegated_to   = g_strdup (mimedir_attribute_get_parameter_value (attribute, "DELEGATED-TO"));
	attendee->priv->delegated_from = g_strdup (mimedir_attribute_get_parameter_value (attribute, "DELEGATED-FROM"));
	attendee->priv->sent_by        = g_strdup (mimedir_attribute_get_parameter_value (attribute, "SENT-BY"));
	attendee->priv->dir            = g_strdup (mimedir_attribute_get_parameter_value (attribute, "DIR"));

	return TRUE;
}

/**
 * mimedir_attendee_write_to_attribute:
 * @attendee: a #MIMEDirAttendee object
 *
 * Write @attendee to a newly allocated #MIMEDirAttribute object. The
 * returned object should be freed with g_object_unref().
 * 
 * Return value: A newly allocated #MIMEDirAttribute object
 **/
MIMEDirAttribute *
mimedir_attendee_write_to_attribute (MIMEDirAttendee *attendee)
{
	MIMEDirAttribute *attr;
	MIMEDirAttendeePriv *priv;

	g_return_val_if_fail (attendee != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTENDEE (attendee), NULL);

	priv = attendee->priv;

	attr = mimedir_attribute_new_with_name ("ATTENDEE");

	mimedir_attribute_set_value (attr, attendee->uri);

	if (priv->cn)
		mimedir_attribute_append_parameter_simple (attr, "CN",             priv->cn);
	if (priv->role)
		mimedir_attribute_append_parameter_simple (attr, "ROLE",           priv->role);
	if (priv->partstat)
		mimedir_attribute_append_parameter_simple (attr, "PARTSTAT",       priv->partstat);
	if (priv->rsvp)
		mimedir_attribute_append_parameter_simple (attr, "RSVP",           priv->rsvp);
	if (priv->cutype)
		mimedir_attribute_append_parameter_simple (attr, "CUTYPE",         priv->cutype);
	if (priv->member)
		mimedir_attribute_append_parameter_simple (attr, "MEMBER",         priv->member);
	if (priv->delegated_to)
		mimedir_attribute_append_parameter_simple (attr, "DELEGATED-TO",   priv->delegated_to);
	if (priv->delegated_from)
		mimedir_attribute_append_parameter_simple (attr, "DELEGATED-FROM", priv->delegated_from);
	if (priv->sent_by)
		mimedir_attribute_append_parameter_simple (attr, "SENT-BY",        priv->sent_by);
	if (priv->dir)
		mimedir_attribute_append_parameter_simple (attr, "DIR",            priv->dir);

	return attr;
}
