/* RFC 2445 vJournal MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vjournal.c 176 2005-02-26 22:46:04Z srittau $
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
#include "mimedir-vjournal.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vjournal_class_init	(MIMEDirVJournalClass	*klass);
static void	 mimedir_vjournal_init		(MIMEDirVJournal		*vjournal);
static void	 mimedir_vjournal_dispose		(GObject		*object);


struct _MIMEDirVJournalPriv {
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_vjournal_get_type (void)
{
	static GType mimedir_vjournal_type = 0;

	if (!mimedir_vjournal_type) {
		static const GTypeInfo mimedir_vjournal_info = {
			sizeof (MIMEDirVJournalClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vjournal_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVJournal),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vjournal_init,
		};

		mimedir_vjournal_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVJournal",
							     &mimedir_vjournal_info,
							     0);
	}

	return mimedir_vjournal_type;
}


static void
mimedir_vjournal_class_init (MIMEDirVJournalClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VJOURNAL_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vjournal_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_vjournal_init (MIMEDirVJournal *vjournal)
{
	MIMEDirVJournalPriv *priv;

	g_return_if_fail (vjournal != NULL);
	g_return_if_fail (MIMEDIR_IS_VJOURNAL (vjournal));

	priv = g_new0 (MIMEDirVJournalPriv, 1);
	vjournal->priv = priv;
}


static void
mimedir_vjournal_dispose (GObject *object)
{
	MIMEDirVJournal *vjournal;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VJOURNAL (object));

	vjournal = MIMEDIR_VJOURNAL (object);

	g_free (vjournal->priv);
	vjournal->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_vjournal_new:
 *
 * Creates a new (empty) vJournal object.
 *
 * Return value: a new vJournal object
 **/
MIMEDirVJournal *
mimedir_vjournal_new (void)
{
	MIMEDirVJournal *vjournal;
	MIMEDirDateTime *now;

	vjournal = g_object_new (MIMEDIR_TYPE_VJOURNAL, NULL);

	now = mimedir_datetime_new_now ();
	g_object_set (G_OBJECT (vjournal), "dtstamp", now, NULL);
	g_object_unref (G_OBJECT (now));

	return vjournal;
}

/**
 * mimedir_vjournal_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vJournal object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vJournal object or %NULL
 **/
MIMEDirVJournal *
mimedir_vjournal_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVJournal *vjournal;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vjournal = g_object_new (MIMEDIR_TYPE_VJOURNAL, NULL);

	if (!mimedir_vjournal_read_from_profile (vjournal, profile, error)) {
		g_object_unref (G_OBJECT (vjournal));
		vjournal = NULL;
	}

	return vjournal;
}

/**
 * mimedir_vjournal_read_from_profile:
 * @vjournal: a vJournal object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vJournal object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vjournal_read_from_profile (MIMEDirVJournal *vjournal, MIMEDirProfile *profile, GError **error)
{
	gchar *name;

	g_return_val_if_fail (vjournal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VJOURNAL (vjournal), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VJOURNAL") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (vjournal), profile, error))
		return FALSE;

	/* Validity Checks */

	/* Normally, a DTSTAMP property is required, but for compatibility
	 * reasons we ignore its lack. (Bah...)
	 */

	return TRUE;
}

/**
 * mimedir_vjournal_write_to_profile:
 * @vjournal: a vjournal
 *
 * Saves the vjournal object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vjournal_write_to_profile (MIMEDirVJournal *vjournal)
{
	g_return_val_if_fail (vjournal != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VJOURNAL (vjournal), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (vjournal), "vJournal");
}

/**
 * mimedir_vjournal_write_to_channel:
 * @vjournal: a vjournal
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vjournal object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vjournal_write_to_channel (MIMEDirVJournal *vjournal, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vjournal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VJOURNAL (vjournal), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vjournal_write_to_profile (vjournal);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vjournal_write_to_string:
 * @vjournal: a vjournal
 *
 * Saves the vjournal object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vjournal_write_to_string (MIMEDirVJournal *vjournal)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vjournal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VJOURNAL (vjournal), FALSE);

	profile = mimedir_vjournal_write_to_profile (vjournal);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}
