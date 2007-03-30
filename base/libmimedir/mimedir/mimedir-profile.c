/* RFC 2425 MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-profile.c 176 2005-02-26 22:46:04Z srittau $
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
#include <errno.h>
#include <unistd.h>

#include <libintl.h>

#include "mimedir-profile.h"
#include "mimedir-utils.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


enum {
	PROP_NAME = 1,
	PROP_CHARSET
};


static void mimedir_profile_class_init	(MIMEDirProfileClass *klass);
static void mimedir_profile_init	(MIMEDirProfile *profile);
static void mimedir_profile_dispose	(GObject *object);
static void mimedir_profile_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void mimedir_profile_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);


struct _MIMEDirProfilePriv {
	GSList	*attributes;
	GSList	*sub_profiles;
	gchar	*name;
	gchar	*charset;
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_profile_get_type (void)
{
	static GType mimedir_profile_type = 0;

	if (!mimedir_profile_type) {
		static const GTypeInfo mimedir_profile_info = {
			sizeof (MIMEDirProfileClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_profile_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirProfile),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_profile_init,
		};

		mimedir_profile_type = g_type_register_static (G_TYPE_OBJECT,
							       "MIMEDirProfile",
							       &mimedir_profile_info,
							       0);
	}

	return mimedir_profile_type;
}


static void
mimedir_profile_class_init (MIMEDirProfileClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_profile_dispose;
	gobject_class->set_property = mimedir_profile_set_property;
	gobject_class->get_property = mimedir_profile_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Properties */

	pspec = g_param_spec_string ("name",
				     _("Profile name"),
				     _("The name (type) of the profile"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NAME, pspec);

	pspec = g_param_spec_string ("charset",
				     _("Character set"),
				     _("Character set used to en-/decode this profile"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CHARSET, pspec);
}

static void
mimedir_profile_init (MIMEDirProfile *profile)
{
	g_return_if_fail (profile != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (profile));

	profile->priv = g_new0 (MIMEDirProfilePriv, 1);
	profile->priv->charset = g_strdup ("utf-8");
}


static void
mimedir_profile_dispose (GObject *object)
{
	MIMEDirProfile *profile;
	MIMEDirProfilePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (object));

	profile = MIMEDIR_PROFILE (object);
	priv = profile->priv;

	if (priv) {
		GSList *item;

		g_free (priv->charset);

		for (item = priv->attributes; item != NULL; item = g_slist_next (item))
			g_object_unref (G_OBJECT (item->data));
		g_slist_free (priv->attributes);

		g_free (priv->name);

		for (item = priv->sub_profiles; item != NULL; item = g_slist_next (item))
			g_object_unref (G_OBJECT (item->data));
		g_slist_free (priv->sub_profiles);

		g_free (priv);
		profile->priv = NULL;
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_profile_set_property (GObject		*object,
			      guint		 property_id,
			      const GValue	*value,
			      GParamSpec	*pspec)
{
	MIMEDirProfile *profile;
	MIMEDirProfilePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (object));

	profile = MIMEDIR_PROFILE (object);
	priv    = profile->priv;

	switch (property_id) {
	case PROP_NAME:
		mimedir_utils_set_property_string (&priv->name, value);
		break;
	case PROP_CHARSET:
		mimedir_utils_set_property_string (&priv->charset, value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}


static void
mimedir_profile_get_property (GObject		*object,
			      guint		 property_id,
			      GValue		*value,
			      GParamSpec	*pspec)
{
	MIMEDirProfile *profile;
	MIMEDirProfilePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (object));

	profile = MIMEDIR_PROFILE (object);
	priv    = profile->priv;

	switch (property_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_CHARSET:
		g_value_set_string (value, priv->charset);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Private Methods
 */

static gboolean
append_attribute (MIMEDirProfile *profile, MIMEDirAttribute *attribute, GError **error)
{
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (!g_ascii_strcasecmp ("BEGIN", mimedir_attribute_get_name (attribute))) {
		const gchar *string;

		if (profile->priv->name) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, "BEGIN");
			return FALSE;
		}

		string = mimedir_attribute_get_value (attribute);

		g_free (profile->priv->name);
		profile->priv->name = g_strdup (string);
		g_strchomp (profile->priv->name);
	}

	g_object_ref (G_OBJECT (attribute));
	profile->priv->attributes = g_slist_append (profile->priv->attributes, attribute);

	return TRUE;
}

/*
 * Public Methods
 */

GQuark
mimedir_profile_error_quark (void)
{
	static gchar qs[] = "mimedir-profile-error-quark";

	return g_quark_from_static_string (qs);
}

/**
 * mimedir_profile_new:
 * @type: the profile type, may be %NULL
 *
 * Creates a new profile of a given type.
 *
 * Return value: pointer to the new profile
 **/
MIMEDirProfile *
mimedir_profile_new (const gchar *type)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (type == NULL || type[0] != '\0', NULL);

	profile = g_object_new (MIMEDIR_TYPE_PROFILE, NULL);

	profile->priv->name = g_strdup (type);

	return profile;
}

/**
 * mimedir_profile_set_charset:
 * @attribute: a #MIMEDirProfile
 * @charset: character set for encoding/decoding this profile
 *
 * Sets the character set that should be used to encode or decode this
 * profile. Defaults to "utf-8".
 */
void
mimedir_profile_set_charset (MIMEDirProfile *profile, const gchar *charset)
{
	g_return_if_fail (profile != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (profile));
	g_return_if_fail (charset != NULL);

	g_free (profile->priv->charset);
	profile->priv->charset = g_strdup (charset);
}

/**
 * mimedir_profile_get_charset:
 * @attribute: a #MIMEDirProfile
 *
 * Returns the character set that is used to encode or decode this profile.
 *
 * Return value: the character set for encoding/decoding this profile
 */
const gchar *
mimedir_profile_get_charset (MIMEDirProfile *profile)
{
	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);

	return profile->priv->charset;
}

/**
 * mimedir_profile_parse:
 * @profile: a #MIMEDirProfile
 * @string: string to parse
 * @error: error storage location or %NULL
 *
 * Clear the supplied profile and re-initialize it with data retrieved from
 * the supplied string. If an error occures while parsing, @error will be set
 * and %FALSE will be returned.
 *
 * Return value: length of consumed string or -1 on error
 */
ssize_t
mimedir_profile_parse (MIMEDirProfile *profile, const gchar *string, GError **error)
{
	MIMEDirAttribute *attribute = NULL;
	const gchar *name;
	ssize_t pos = 0;
	GSList *profile_stack = NULL;
	GSList *item;

	g_return_val_if_fail (profile != NULL, -1);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), -1);
	g_return_val_if_fail (string != NULL, -1);
	g_return_val_if_fail (error == NULL || *error == NULL, -1);

	/* Read profile. If the first attribute is a BEGIN, read until reaching
	 * the matching END attribute. Otherwise read to EOF.
	 */

	/* Read the first attribute */

	attribute = mimedir_attribute_new ();
	mimedir_attribute_set_charset (attribute, profile->priv->charset);
	pos = mimedir_attribute_parse (attribute, string, error);
	if (pos < 0) {
		g_object_unref (G_OBJECT (attribute));
		return -1;
	}

	if (!append_attribute (profile, attribute, error)) {
		g_object_unref (G_OBJECT (attribute));
		return -1;
	}

	g_object_unref (G_OBJECT (attribute));

	/* Iterate over the attributes */

	g_object_ref (G_OBJECT (profile));

	for (;;) {
		ssize_t newpos;

		/* Read next attribute */

		attribute = mimedir_attribute_new ();
		mimedir_attribute_set_charset (attribute, profile->priv->charset);
		newpos = mimedir_attribute_parse (attribute, string + pos, error);
		if (newpos < 0) {
			g_object_unref (G_OBJECT (attribute));
			pos = -1;
			break;
		} else if (newpos == 0) {
			if (profile->priv->name || profile_stack) { /* premature EOF */
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_UNEXPECTED_END, MIMEDIR_PROFILE_ERROR_UNEXPECTED_END_STR);
				pos = -1;
			}
			break;
		}
		pos += newpos;

		/* Check whether it's a BEGIN or END attribute */

		name = mimedir_attribute_get_name (attribute);
		if (!g_ascii_strcasecmp (name, "BEGIN")) {
			/* Read nested profile */

			profile_stack = g_slist_prepend (profile_stack, profile);

			profile = mimedir_profile_new (NULL);

			/* Append attribute */

			if (!append_attribute (profile, attribute, error)) {
				g_object_unref (G_OBJECT (attribute));
				pos = -1;
				break;
			}

		} else if (!g_ascii_strcasecmp (name, "END")) {
			MIMEDirProfile *old_profile;
			gchar *v;

			v = mimedir_attribute_get_value_text (attribute, error);
			if (!v) {
				g_object_unref (G_OBJECT (attribute));
				pos = -1;
				break;
			}

			if (!profile->priv->name ||
			    !mimedir_utils_compare_tokens (profile->priv->name, v)) {
				g_object_unref (G_OBJECT (attribute));
				g_free (v);
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_UNMATCHED_END, MIMEDIR_PROFILE_ERROR_UNMATCHED_END_STR);
				pos = -1;
				break;
			}

			g_free (v);

			/* Success: append attribute */

			if (!append_attribute (profile, attribute, error)) {
				g_object_unref (G_OBJECT (attribute));
				pos = -1;
				break;
			}

			/* Finish this profile */

			if (!profile_stack)
				break;

			old_profile = profile;

			profile = MIMEDIR_PROFILE (profile_stack->data);

			profile_stack = g_slist_delete_link (profile_stack, profile_stack);

			mimedir_profile_append_subprofile (profile, old_profile);
			g_object_unref (G_OBJECT (old_profile));
		} else {
			/* Append attribute */

			if (!append_attribute (profile, attribute, error)) {
				g_object_unref (G_OBJECT (attribute));
				pos = -1;
				break;
			}
		}
	}

	g_object_unref (G_OBJECT (profile));

	for (item = profile_stack; item != NULL; item = g_slist_next (item))
		g_object_unref (G_OBJECT (item->data));
	g_slist_free (profile_stack);

	return pos;
}

/**
 * mimedir_profile_write_to_channel:
 * @profile: a #MIMEDirProfile
 * @channel: I/O channel to write to
 * @error: error storage location or %NULL
 *
 * Writes the profile to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_profile_write_to_channel (MIMEDirProfile *profile, GIOChannel *channel, GError **error)
{
	MIMEDirProfilePriv *priv;
	GSList *l;

	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = profile->priv;

	if (priv->name) {
		if (g_io_channel_write_chars (channel, "BEGIN:", -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, priv->name, -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, "\n", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
	}

	for (l = priv->attributes; l != NULL; l = g_slist_next (l)) {
		MIMEDirAttribute *attr;

		g_assert (l->data != NULL && MIMEDIR_IS_ATTRIBUTE (l->data));
		attr = MIMEDIR_ATTRIBUTE (l->data);

		if (!mimedir_attribute_write_to_channel (attr, channel, error))
			return FALSE;
	}

	for (l = priv->sub_profiles; l != NULL; l = g_slist_next (l)) {
		MIMEDirProfile *prof;

		g_assert (l->data != NULL && MIMEDIR_IS_PROFILE (l->data));
		prof = MIMEDIR_PROFILE (l->data);

		if (g_io_channel_write_chars (channel, "\n", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;

		if (!mimedir_profile_write_to_channel (prof, channel, error))
			return FALSE;
	}

	if (priv->name) {
		if (g_io_channel_write_chars (channel, "END:", -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, priv->name, -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, "\n", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
	}

	return TRUE;
}

/**
 * mimedir_profile_write_to_string:
 * @profile: a #MIMEDirProfile
 *
 * Saves the profile to a newly allocated string. It should be freed with
 * g_free().
 *
 * Return value: a string
 **/
gchar *
mimedir_profile_write_to_string (MIMEDirProfile *profile)
{
	GError *error = NULL;
	GIOChannel *channel;
	gchar *tmpname = NULL;
	gchar *buffer;
	int fd;

	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);

	/* Write profile to temp file */

	fd = g_file_open_tmp (NULL, &tmpname, &error);
	if (fd < 0) {
		g_free (tmpname);
		g_warning (error->message);
		g_error_free (error);
		return g_strdup ("");
	}

	channel = g_io_channel_unix_new (fd);
	if (!mimedir_profile_write_to_channel (profile, channel, &error)) {
		g_free (tmpname);
		g_io_channel_unref (channel);
		close(fd);
		g_warning (error->message);
		g_error_free (error);
		return g_strdup ("");
	}
	g_io_channel_unref (channel);
	close(fd);

	if (!g_file_get_contents (tmpname, &buffer, NULL, &error)) {
		g_free (tmpname);
		g_warning (error->message);
		g_error_free (error);
		return g_strdup ("");
	}

	g_unlink(tmpname);
	g_free (tmpname);

	return buffer;
}

/**
 * mimedir_profile_append_attribute:
 * @profile: a #MIMEDirProfile
 * @attribute: attribute to add
 *
 * Adds an attribute to the supplied profile.
 **/
void
mimedir_profile_append_attribute (MIMEDirProfile *profile, MIMEDirAttribute *attribute)
{
	g_return_if_fail (profile != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (profile));
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	g_object_ref (G_OBJECT (attribute));
	profile->priv->attributes = g_slist_append (profile->priv->attributes, attribute);
}

/**
 * mimedir_profile_get_attribute
 * @profile: a #MIMEDirProfile
 * @name: attribute name
 *
 * Tries to find the first occurence of a given attribute by @name and
 * return the MIMEDirAttribute object. If no attribute with the given
 * name is found, %NULL is returned.
 *
 * Return value: the attribute or %NULL
 **/
MIMEDirAttribute *
mimedir_profile_get_attribute (MIMEDirProfile *profile, const gchar *name)
{
	GSList *current;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (name != NULL, NULL);

	for (current = profile->priv->attributes; current != NULL; current = g_slist_next (current)) {
		const gchar *attname;

		attname = mimedir_attribute_get_name (MIMEDIR_ATTRIBUTE (current->data));
		if (!g_ascii_strcasecmp (name, attname))
			return MIMEDIR_ATTRIBUTE (current->data);
	}

	/* Not found */

	return NULL;
}

/**
 * mimedir_profile_get_attribute_list:
 * @profile: a #MIMEDirProfile
 *
 * Return the ordered list of all the profile's attributes.
 *
 * Return value: the attribute list
 **/
GSList *
mimedir_profile_get_attribute_list (MIMEDirProfile *profile)
{
	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);

	return profile->priv->attributes;
}

/**
 * mimedir_profile_append_subprofile:
 * @profile: a #MIMEDirProfile
 * @sub_profile: a #MIMEDirProfile that's to be appended
 *
 * Appends @sub_profile to @profile as a sub-profile.
 **/
void
mimedir_profile_append_subprofile (MIMEDirProfile *profile, MIMEDirProfile *sub_profile)
{
	MIMEDirProfilePriv *priv;

	g_return_if_fail (profile != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (profile));
	g_return_if_fail (sub_profile != NULL);
	g_return_if_fail (MIMEDIR_IS_PROFILE (sub_profile));

	priv = profile->priv;

	g_object_ref (G_OBJECT (sub_profile));
	priv->sub_profiles = g_slist_append (priv->sub_profiles, sub_profile);
}

/**
 * mimedir_profile_get_subprofiles:
 * @profile: a #MIMEDirProfile
 *
 * Returns the list of all sub-profiles of @profile. The returned list
 * must not be altered in any way!
 *
 * Return value: #GSList containing all sub-profiles
 **/
GSList *
mimedir_profile_get_subprofiles	(MIMEDirProfile *profile)
{
	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);

	return profile->priv->sub_profiles;
}
