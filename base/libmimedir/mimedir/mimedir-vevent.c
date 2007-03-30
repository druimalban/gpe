/* RFC 2445 vEvent MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vevent.c 176 2005-02-26 22:46:04Z srittau $
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
#include "mimedir-vcomponent.h"
#include "mimedir-vevent.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vevent_class_init	(MIMEDirVEventClass	*klass);
static void	 mimedir_vevent_init		(MIMEDirVEvent		*vevent);
static void	 mimedir_vevent_dispose		(GObject		*object);


struct _MIMEDirVEventPriv {
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_vevent_get_type (void)
{
	static GType mimedir_vevent_type = 0;

	if (!mimedir_vevent_type) {
		static const GTypeInfo mimedir_vevent_info = {
			sizeof (MIMEDirVEventClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vevent_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVEvent),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vevent_init,
		};

		mimedir_vevent_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVEvent",
							     &mimedir_vevent_info,
							     0);
	}

	return mimedir_vevent_type;
}


static void
mimedir_vevent_class_init (MIMEDirVEventClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VEVENT_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose = mimedir_vevent_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_vevent_init (MIMEDirVEvent *vevent)
{
	MIMEDirVEventPriv *priv;

	g_return_if_fail (vevent != NULL);
	g_return_if_fail (MIMEDIR_IS_VEVENT (vevent));

	priv = g_new0 (MIMEDirVEventPriv, 1);
	vevent->priv = priv;
}


static void
mimedir_vevent_dispose (GObject *object)
{
	MIMEDirVEvent *vevent;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VEVENT (object));

	vevent = MIMEDIR_VEVENT (object);

	g_free (vevent->priv);
	vevent->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_vevent_new:
 *
 * Creates a new (empty) vevent object.
 *
 * Return value: a new vevent object
 **/
MIMEDirVEvent *
mimedir_vevent_new (void)
{
	MIMEDirVEvent *vevent;
	MIMEDirDateTime *now;

	vevent = g_object_new (MIMEDIR_TYPE_VEVENT, NULL);

	now = mimedir_datetime_new_now ();
	g_object_set (G_OBJECT (vevent), "dtstamp", now, NULL);
	g_object_unref (G_OBJECT (now));

	return vevent;
}

/**
 * mimedir_vevent_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vEvent object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vEvent object or %NULL
 **/
MIMEDirVEvent *
mimedir_vevent_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVEvent *vevent;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vevent = g_object_new (MIMEDIR_TYPE_VEVENT, NULL);

	if (!mimedir_vevent_read_from_profile (vevent, profile, error)) {
		g_object_unref (G_OBJECT (vevent));
		vevent = NULL;
	}

	return vevent;
}

/**
 * mimedir_vevent_read_from_profile:
 * @vevent: a vEvent object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vEvent object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vevent_read_from_profile (MIMEDirVEvent *vevent, MIMEDirProfile *profile, GError **error)
{
	gchar *name;

	g_return_val_if_fail (vevent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VEVENT (vevent), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VEVENT") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (vevent), profile, error))
		return FALSE;

	/* Validity Checks */

	/* Normally, a DTSTAMP property is required, but for compatibility
	 * reasons we ignore its lack. (Bah...)
	 */

	return TRUE;
}

/**
 * mimedir_vevent_write_to_profile:
 * @vevent: a #MIMEDirVEvent object
 *
 * Saves the vEvent object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vevent_write_to_profile (MIMEDirVEvent *vevent)
{
	g_return_val_if_fail (vevent != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VEVENT (vevent), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (vevent), "vEvent");
}

/**
 * mimedir_vevent_write_to_channel:
 * @vevent: a #MIMEDirVEvent object
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vEvent object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vevent_write_to_channel (MIMEDirVEvent *vevent, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vevent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VEVENT (vevent), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vevent_write_to_profile (vevent);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vevent_write_to_string:
 * @vevent: a #MIMEDirVEvent object
 *
 * Saves the vevent object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vevent_write_to_string (MIMEDirVEvent *vevent)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vevent != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VEVENT (vevent), FALSE);

	profile = mimedir_vevent_write_to_profile (vevent);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}
