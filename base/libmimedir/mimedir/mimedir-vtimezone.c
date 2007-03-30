/* RFC 2445 vTimeZone MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vtimezone.c 176 2005-02-26 22:46:04Z srittau $
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
#include "mimedir-vtimezone.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vtimezone_class_init	(MIMEDirVTimeZoneClass	*klass);
static void	 mimedir_vtimezone_init		(MIMEDirVTimeZone		*vtimezone);
static void	 mimedir_vtimezone_dispose		(GObject		*object);


struct _MIMEDirVTimeZonePriv {
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_vtimezone_get_type (void)
{
	static GType mimedir_vtimezone_type = 0;

	if (!mimedir_vtimezone_type) {
		static const GTypeInfo mimedir_vtimezone_info = {
			sizeof (MIMEDirVTimeZoneClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vtimezone_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVTimeZone),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vtimezone_init,
		};

		mimedir_vtimezone_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVTimeZone",
							     &mimedir_vtimezone_info,
							     0);
	}

	return mimedir_vtimezone_type;
}


static void
mimedir_vtimezone_class_init (MIMEDirVTimeZoneClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VTIMEZONE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vtimezone_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_vtimezone_init (MIMEDirVTimeZone *vtimezone)
{
	MIMEDirVTimeZonePriv *priv;

	g_return_if_fail (vtimezone != NULL);
	g_return_if_fail (MIMEDIR_IS_VTIMEZONE (vtimezone));

	priv = g_new0 (MIMEDirVTimeZonePriv, 1);
	vtimezone->priv = priv;
}


static void
mimedir_vtimezone_dispose (GObject *object)
{
	MIMEDirVTimeZone *vtimezone;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VTIMEZONE (object));

	vtimezone = MIMEDIR_VTIMEZONE (object);

	g_free (vtimezone->priv);
	vtimezone->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_vtimezone_new:
 *
 * Creates a new (empty) vTimeZone object.
 *
 * Return value: a new vTimeZone object
 **/
MIMEDirVTimeZone *
mimedir_vtimezone_new (void)
{
	MIMEDirVTimeZone *vtimezone;

	vtimezone = g_object_new (MIMEDIR_TYPE_VTIMEZONE, NULL);

	return vtimezone;
}

/**
 * mimedir_vtimezone_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vTimeZone object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vTimeZone object or %NULL
 **/
MIMEDirVTimeZone *
mimedir_vtimezone_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVTimeZone *vtimezone;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vtimezone = g_object_new (MIMEDIR_TYPE_VTIMEZONE, NULL);

	if (!mimedir_vtimezone_read_from_profile (vtimezone, profile, error)) {
		g_object_unref (G_OBJECT (vtimezone));
		vtimezone = NULL;
	}

	return vtimezone;
}

/**
 * mimedir_vtimezone_read_from_profile:
 * @vtimezone: a vTimeZone object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vTimeZone object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vtimezone_read_from_profile (MIMEDirVTimeZone *vtimezone, MIMEDirProfile *profile, GError **error)
{
	gchar *name;

	g_return_val_if_fail (vtimezone != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTIMEZONE (vtimezone), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VTIMEZONE") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (vtimezone), profile, error))
		return FALSE;

	/* Validity Checks */

	/* Normally, a DTSTAMP property is required, but for compatibility
	 * reasons we ignore its lack. (Bah...)
	 */

	return TRUE;
}

/**
 * mimedir_vtimezone_write_to_profile:
 * @vtimezone: a vtimezone
 *
 * Saves the vtimezone object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vtimezone_write_to_profile (MIMEDirVTimeZone *vtimezone)
{
	g_return_val_if_fail (vtimezone != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VTIMEZONE (vtimezone), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (vtimezone), "vTimeZone");
}

/**
 * mimedir_vtimezone_write_to_channel:
 * @vtimezone: a vtimezone
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vtimezone object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vtimezone_write_to_channel (MIMEDirVTimeZone *vtimezone, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vtimezone != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTIMEZONE (vtimezone), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vtimezone_write_to_profile (vtimezone);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vtimezone_write_to_string:
 * @vtimezone: a vtimezone
 *
 * Saves the vtimezone object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vtimezone_write_to_string (MIMEDirVTimeZone *vtimezone)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vtimezone != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTIMEZONE (vtimezone), FALSE);

	profile = mimedir_vtimezone_write_to_profile (vtimezone);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}
