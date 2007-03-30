/* RFC 2445 vTodo MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vtodo.c 176 2005-02-26 22:46:04Z srittau $
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
#include "mimedir-vtodo.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vtodo_class_init	(MIMEDirVTodoClass	*klass);
static void	 mimedir_vtodo_init		(MIMEDirVTodo		*vtodo);
static void	 mimedir_vtodo_dispose		(GObject		*object);


struct _MIMEDirVTodoPriv {
};

static MIMEDirVComponentClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_vtodo_get_type (void)
{
	static GType mimedir_vtodo_type = 0;

	if (!mimedir_vtodo_type) {
		static const GTypeInfo mimedir_vtodo_info = {
			sizeof (MIMEDirVTodoClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vtodo_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVTodo),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vtodo_init,
		};

		mimedir_vtodo_type = g_type_register_static (MIMEDIR_TYPE_VCOMPONENT,
							     "MIMEDirVTodo",
							     &mimedir_vtodo_info,
							     0);
	}

	return mimedir_vtodo_type;
}


static void
mimedir_vtodo_class_init (MIMEDirVTodoClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VTODO_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vtodo_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_vtodo_init (MIMEDirVTodo *vtodo)
{
	MIMEDirVTodoPriv *priv;

	g_return_if_fail (vtodo != NULL);
	g_return_if_fail (MIMEDIR_IS_VTODO (vtodo));

	priv = g_new0 (MIMEDirVTodoPriv, 1);
	vtodo->priv = priv;
}


static void
mimedir_vtodo_dispose (GObject *object)
{
	MIMEDirVTodo *vtodo;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VTODO (object));

	vtodo = MIMEDIR_VTODO (object);

	g_free (vtodo->priv);
	vtodo->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_vtodo_new:
 *
 * Creates a new (empty) vTodo object.
 *
 * Return value: a new vTodo object
 **/
MIMEDirVTodo *
mimedir_vtodo_new (void)
{
	MIMEDirVTodo *vtodo;
	MIMEDirDateTime *now;

	vtodo = g_object_new (MIMEDIR_TYPE_VTODO, NULL);

	now = mimedir_datetime_new_now ();
	g_object_set (G_OBJECT (vtodo), "dtstamp", now, NULL);
	g_object_unref (G_OBJECT (now));

	return vtodo;
}

/**
 * mimedir_vtodo_new_from_profile:
 * @profile: a #MIMEDirProfile object
 * @error: error storage location or %NULL
 *
 * Create a new vTodo object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vTodo object or %NULL
 **/
MIMEDirVTodo *
mimedir_vtodo_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVTodo *vtodo;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vtodo = g_object_new (MIMEDIR_TYPE_VTODO, NULL);

	if (!mimedir_vtodo_read_from_profile (vtodo, profile, error)) {
		g_object_unref (G_OBJECT (vtodo));
		vtodo = NULL;
	}

	return vtodo;
}

/**
 * mimedir_vtodo_read_from_profile:
 * @vtodo: a vTodo object
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vTodo object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vtodo_read_from_profile (MIMEDirVTodo *vtodo, MIMEDirProfile *profile, GError **error)
{
	gchar *name;

	g_return_val_if_fail (vtodo != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTODO (vtodo), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VTODO") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	if (!mimedir_vcomponent_read_from_profile (MIMEDIR_VCOMPONENT (vtodo), profile, error))
		return FALSE;

	/* Validity Checks */

	/* Normally, a DTSTAMP property is required, but for compatibility
	 * reasons we ignore its lack. (Bah...)
	 */

	return TRUE;
}

/**
 * mimedir_vtodo_write_to_profile:
 * @vtodo: a vtodo
 *
 * Saves the vtodo object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vtodo_write_to_profile (MIMEDirVTodo *vtodo)
{
	g_return_val_if_fail (vtodo != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VTODO (vtodo), NULL);

	return mimedir_vcomponent_write_to_profile (MIMEDIR_VCOMPONENT (vtodo), "vTodo");
}

/**
 * mimedir_vtodo_write_to_channel:
 * @vtodo: a vtodo
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vtodo object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vtodo_write_to_channel (MIMEDirVTodo *vtodo, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vtodo != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTODO (vtodo), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vtodo_write_to_profile (vtodo);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vtodo_write_to_string:
 * @vtodo: a vtodo
 *
 * Saves the vtodo object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vtodo_write_to_string (MIMEDirVTodo *vtodo)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vtodo != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VTODO (vtodo), FALSE);

	profile = mimedir_vtodo_write_to_profile (vtodo);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}
