/* RFC 2445 vFreeBusy MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vfreebusy.c 176 2005-02-26 22:46:04Z srittau $
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
#include "mimedir-vfreebusy.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vfreebusy_class_init	(MIMEDirVFreeBusyClass	*klass);
static void	 mimedir_vfreebusy_init		(MIMEDirVFreeBusy		*vfreebusy);
static void	 mimedir_vfreebusy_dispose		(GObject		*object);


struct _MIMEDirVFreeBusyPriv {
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_vfreebusy_get_type (void)
{
	static GType mimedir_vfreebusy_type = 0;

	if (!mimedir_vfreebusy_type) {
		static const GTypeInfo mimedir_vfreebusy_info = {
			sizeof (MIMEDirVFreeBusyClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vfreebusy_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVFreeBusy),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vfreebusy_init,
		};

		mimedir_vfreebusy_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVFreeBusy",
							     &mimedir_vfreebusy_info,
							     0);
	}

	return mimedir_vfreebusy_type;
}


static void
mimedir_vfreebusy_class_init (MIMEDirVFreeBusyClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VFREEBUSY_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vfreebusy_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_vfreebusy_init (MIMEDirVFreeBusy *vfreebusy)
{
	MIMEDirVFreeBusyPriv *priv;

	g_return_if_fail (vfreebusy != NULL);
	g_return_if_fail (MIMEDIR_IS_VFREEBUSY (vfreebusy));

	priv = g_new0 (MIMEDirVFreeBusyPriv, 1);
	vfreebusy->priv = priv;
}


static void
mimedir_vfreebusy_dispose (GObject *object)
{
	MIMEDirVFreeBusy *vfreebusy;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VFREEBUSY (object));

	vfreebusy = MIMEDIR_VFREEBUSY (object);

	g_free (vfreebusy->priv);
	vfreebusy->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_vfreebusy_new:
 *
 * Creates a new (empty) vFreeBusy object.
 *
 * Return value: a new vFreeBusy object
 **/
MIMEDirVFreeBusy *
mimedir_vfreebusy_new (void)
{
	MIMEDirVFreeBusy *vfreebusy;
	MIMEDirDateTime *now;

	vfreebusy = g_object_new (MIMEDIR_TYPE_VFREEBUSY, NULL);

	now = mimedir_datetime_new_now ();
	g_object_set (G_OBJECT (vfreebusy), "dtstamp", now, NULL);
	g_object_unref (G_OBJECT (now));

	return vfreebusy;
}

/**
 * mimedir_vfreebusy_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vFreeBusy object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vFreeBusy object or %NULL
 **/
MIMEDirVFreeBusy *
mimedir_vfreebusy_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVFreeBusy *vfreebusy;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vfreebusy = g_object_new (MIMEDIR_TYPE_VFREEBUSY, NULL);

	if (!mimedir_vfreebusy_read_from_profile (vfreebusy, profile, error)) {
		g_object_unref (G_OBJECT (vfreebusy));
		vfreebusy = NULL;
	}

	return vfreebusy;
}

/**
 * mimedir_vfreebusy_read_from_profile:
 * @vfreebusy: a vFreeBusy object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vFreeBusy object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vfreebusy_read_from_profile (MIMEDirVFreeBusy *vfreebusy, MIMEDirProfile *profile, GError **error)
{
	gchar *name;

	g_return_val_if_fail (vfreebusy != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VFREEBUSY (vfreebusy), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VFREEBUSY") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (vfreebusy), profile, error))
		return FALSE;

	/* Validity Checks */

	/* Normally, a DTSTAMP property is required, but for compatibility
	 * reasons we ignore its lack. (Bah...)
	 */

	return TRUE;
}

/**
 * mimedir_vfreebusy_write_to_profile:
 * @vfreebusy: a vfreebusy
 *
 * Saves the vfreebusy object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vfreebusy_write_to_profile (MIMEDirVFreeBusy *vfreebusy)
{
	g_return_val_if_fail (vfreebusy != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VFREEBUSY (vfreebusy), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (vfreebusy), "vFreeBusy");
}

/**
 * mimedir_vfreebusy_write_to_channel:
 * @vfreebusy: a vfreebusy
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vfreebusy object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vfreebusy_write_to_channel (MIMEDirVFreeBusy *vfreebusy, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vfreebusy != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VFREEBUSY (vfreebusy), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vfreebusy_write_to_profile (vfreebusy);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vfreebusy_write_to_string:
 * @vfreebusy: a vfreebusy
 *
 * Saves the vfreebusy object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vfreebusy_write_to_string (MIMEDirVFreeBusy *vfreebusy)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vfreebusy != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VFREEBUSY (vfreebusy), FALSE);

	profile = mimedir_vfreebusy_write_to_profile (vfreebusy);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}
