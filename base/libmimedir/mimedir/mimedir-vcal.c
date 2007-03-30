/* RFC 2445 iCalendar MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcal.c 185 2005-03-01 03:01:49Z srittau $
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

#include <time.h>

#include <libintl.h>

#include "mimedir-marshal.h"
#include "mimedir-profile.h"
#include "mimedir-utils.h"
#include "mimedir-valarm.h"
#include "mimedir-vcal.h"
#include "mimedir-vevent.h"
#include "mimedir-vfreebusy.h"
#include "mimedir-vjournal.h"
#include "mimedir-vtimezone.h"
#include "mimedir-vtodo.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcal_class_init	(MIMEDirVCalClass	*klass);
static void	 mimedir_vcal_init		(MIMEDirVCal		*vcal);
static void	 mimedir_vcal_dispose		(GObject		*object);
static void	 mimedir_vcal_set_property	(GObject		*object,
						 guint			 property_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 mimedir_vcal_get_property	(GObject		*object,
						 guint			 property_id,
						 GValue			*value,
						 GParamSpec		*pspec);

static void	 mimedir_vcal_clear		(MIMEDirVCal		*vcal);
static void	 mimedir_vcal_changed		(MIMEDirVCal		*vcal);


enum {
	PROP_METHOD = 1,
	PROP_PRODID
};

enum {
	SIGNAL_CHANGED,
	SIGNAL_LAST
};

struct _MIMEDirVCalPriv {
	gboolean	 changed;

	GList		*components;

	gchar		*method;	/* Method (4.7.2) */
	gchar		*prodid;	/* Product Identifier (4.7.3) */
};

static MIMEDirProfileClass *parent_class = NULL;

static gint mimedir_vcal_signals[SIGNAL_LAST] = { 0 };

/*
 * Class and Object Management
 */

GType
mimedir_vcal_get_type (void)
{
	static GType mimedir_vcal_type = 0;

	if (!mimedir_vcal_type) {
		static const GTypeInfo mimedir_vcal_info = {
			sizeof (MIMEDirVCalClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcal_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVCal),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcal_init,
		};

		mimedir_vcal_type = g_type_register_static (G_TYPE_OBJECT,
							    "MIMEDirVCal",
							    &mimedir_vcal_info,
							    0);
	}

	return mimedir_vcal_type;
}


static void
mimedir_vcal_class_init (MIMEDirVCalClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vcal_dispose;
	gobject_class->set_property = mimedir_vcal_set_property;
	gobject_class->get_property = mimedir_vcal_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcal_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVCalClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_string ("method",
				     _("Method"),
				     _("The vCalendar's method"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_METHOD, pspec);
	pspec = g_param_spec_string ("prodid",
				     _("Product identifier"),
				     _("The product identifier of the vCalendar. In case of newly generated objects, this is LibMIMEDir's product identifier"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PRODID, pspec);
}


static void
mimedir_vcal_init (MIMEDirVCal *vcal)
{
	g_return_if_fail (vcal != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (vcal));

	vcal->priv = g_new0 (MIMEDirVCalPriv, 1);

	vcal->priv->changed = FALSE;
}


static void
mimedir_vcal_dispose (GObject *object)
{
	MIMEDirVCal     *vcal;
	MIMEDirVCalPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (object));

	vcal = MIMEDIR_VCAL (object);

	priv = vcal->priv;

	if (priv)
		mimedir_vcal_clear (vcal);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_vcal_set_property (GObject		*object,
			    guint		 property_id,
			    const GValue	*value,
			    GParamSpec		*pspec)
{
	MIMEDirVCal *vcal;
	MIMEDirVCalPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (object));

	vcal = MIMEDIR_VCAL (object);
	priv = vcal->priv;

	switch (property_id) {
	case PROP_METHOD:
		mimedir_utils_set_property_string (&priv->method, value);
		break;
	case PROP_PRODID:
		mimedir_utils_set_property_string (&priv->prodid, value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	mimedir_vcal_changed (vcal);
}


static void
mimedir_vcal_get_property (GObject	*object,
			    guint	 property_id,
			    GValue	*value,
			    GParamSpec	*pspec)
{
	MIMEDirVCal *vcal;
	MIMEDirVCalPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (object));

	vcal = MIMEDIR_VCAL (object);
	priv = vcal->priv;

	switch (property_id) {
	case PROP_METHOD:
		g_value_set_string (value, priv->method);
		break;
	case PROP_PRODID:
		g_value_set_string (value, priv->prodid);
		break;

 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Private Methods
 */

/* Free VCal's allocated memory */
static void
mimedir_vcal_clear (MIMEDirVCal *vcal)
{
	MIMEDirVCalPriv *priv;
	GList *l;

	priv = vcal->priv;

	if (!priv)
		return;

	for (l = priv->components; l != NULL; l = g_list_next (l))
		g_object_unref (G_OBJECT (l->data));
	g_list_free (priv->components);

	g_free (priv->method);
	g_free (priv->prodid);

	g_free (priv);
	vcal->priv = NULL;
}


/* Reallocate memory for the VCal */
static void
mimedir_vcal_reset (MIMEDirVCal *vcal)
{
	mimedir_vcal_clear (vcal);
	mimedir_vcal_init (vcal);
}


static gboolean
mimedir_vcal_parse_attribute (MIMEDirVCal *vcal, MIMEDirAttribute *attr, GError **error)
{
	MIMEDirVCalPriv *priv;
	const gchar *name;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = vcal->priv;

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
	 * Calendar Properties (RFC 2445. Section 4)
	 */

	/* Calendar Scale (4.7.1) */

	else if (!g_ascii_strcasecmp (name, "CALSCALE")) {
		gchar *t;

		t = mimedir_attribute_get_value_text (attr, error);
		if (!t)
			return FALSE;

		g_strstrip (t);
		if (g_ascii_strcasecmp (t, "GREGORIAN") != 0) {
			g_set_error (error, MIMEDIR_VCAL_ERROR, MIMEDIR_VCAL_ERROR_UNKNOWN_SCALE, MIMEDIR_VCAL_ERROR_UNKNOWN_SCALE_STR, t);
			g_free (t);
			return FALSE;
		}

		g_free (t);
	}

	/* Method (4.7.2) */

	else if (!g_ascii_strcasecmp (name, "METHOD")) {
		if (priv->method) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name);
			return FALSE;
		}

		priv->method = mimedir_attribute_get_value_text (attr, error);
		if (!priv->method)
			return FALSE;
	}

	/* Product Identifier (4.7.3) */

	else if (!g_ascii_strcasecmp (name, "PRODID")) {
		if (priv->prodid) {
			g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name);
			return FALSE;
		}

		priv->prodid = mimedir_attribute_get_value_text (attr, error);
		if (!priv->prodid)
			return FALSE;
	}

	/* Version (4.7.4) */

	else if (!g_ascii_strcasecmp (name, "VERSION")) {
		/* just ignore */
	}

	/*
	 * Custom Properties
	 */

	else if (!g_ascii_strcasecmp (name, "TZ")) { /* compat */
		/* ignore */
	}

	else if (!g_ascii_strncasecmp (name, "X-", 2))
		g_printerr (_("The profile contains the unsupported custom attribute %s.\n"), name);

	else
		g_printerr (_("The profile contains the unknown attribute %s.\n"), name);

	return TRUE;
}



/* Emit the changed signal */
static void
mimedir_vcal_changed (MIMEDirVCal *vcal)
{
	vcal->priv->changed = TRUE;
	g_signal_emit (G_OBJECT (vcal), mimedir_vcal_signals[SIGNAL_CHANGED], 0);
}

/*
 * Public Methods
 */

GQuark
mimedir_vcal_error_quark (void)
{
	static gchar qs[] = "mimedir-vcal-error-quark";

	return g_quark_from_static_string (qs);
}


/* This function is documented elsewhere. */
GList *
mimedir_vcal_read_file (const gchar *filename, GError **error)
{
	GError *err = NULL;
	GIOChannel *channel;
	GList *list;

	g_return_val_if_fail (filename != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Try to read file as UTF-8 file */

	channel = g_io_channel_new_file (filename, "r", error);
	if (!channel) {
		if (error && *error) {
			gchar *msg = (*error)->message;
			(*error)->message = g_strdup_printf (_("Error reading calendar file %s: %s!"), filename, msg);
			g_free (msg);
		}
		return NULL;
	}

	list = mimedir_vcal_read_channel (channel, &err);

	g_io_channel_unref (channel);

	/* Try again opening the file with the current locale if opening
	 * it as UTF-8 coded failed.
	 */

	if (err && err->domain == G_CONVERT_ERROR) {
		g_error_free (err);
		err = NULL;

		channel = g_io_channel_new_file (filename, "r", error);
		if (!channel) {
			if (error && *error) {
				gchar *msg = (*error)->message;
				(*error)->message = g_strdup_printf (_("Error reading calendar file %s: %s!"), filename, msg);
				g_free (msg);
			}
			return NULL;
		}

		if (g_io_channel_set_encoding (channel, NULL, error) != G_IO_STATUS_NORMAL) {
			g_io_channel_unref (channel);
			return NULL;
		}

		list = mimedir_vcal_read_channel (channel, &err);

		g_io_channel_unref (channel);
	}

	/* Try again opening the file as ISO-8859-15 if opening with current
	 * locale failed.
	 */

	if (err && err->domain == G_CONVERT_ERROR) {
		g_error_free (err);
		err = NULL;

		channel = g_io_channel_new_file (filename, "r", error);
		if (!channel) {
			if (error && *error) {
				gchar *msg = (*error)->message;
				(*error)->message = g_strdup_printf (_("Error reading calendar file %s: %s!"), filename, msg);
				g_free (msg);
			}
			return NULL;
		}

		if (g_io_channel_set_encoding (channel, "ISO-8859-15", error) != G_IO_STATUS_NORMAL) {
			g_io_channel_unref (channel);
			return NULL;
		}

		list = mimedir_vcal_read_channel (channel, &err);

		g_io_channel_unref (channel);
	}

	/* Error handling */

	if (err) {
		gchar *msg;

		g_propagate_error (error, err);

		msg = (*error)->message;
		(*error)->message = g_strdup_printf (_("Error reading calendar file %s: %s!"), filename, msg);
		g_free (msg);
	}

	return list;
}

/**
 * mimedir_vcal_read_channel:
 * @channel: I/O channel to read from
 * @error: error storage location or %NULL
 *
 * Reads a list of vcals from the supplied I/O channel and returns it. @error
 * will be set if an error occurs. You should free the returned list with
 * mimedir_vcal_free_list().
 *
 * Return value: list of read vcals
 **/
GList *
mimedir_vcal_read_channel (GIOChannel *channel, GError **error)
{
	GList *list = NULL;

	GIOStatus status;

	gchar *string;
	gsize len, pos;

	g_return_val_if_fail (channel != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	status = g_io_channel_read_to_end (channel, &string, &len, error);
	if (status != G_IO_STATUS_NORMAL)
		return NULL;

	for (pos = 0; pos < len;) {
		MIMEDirVCal *vcal;
		MIMEDirProfile *profile;
		const gchar *charset;
		ssize_t newpos;

		/* Skip white space */

		for (; g_ascii_isspace (string[pos]); pos++)
			;
		if (pos == len)
			break;

		/* Read profile */

		profile = mimedir_profile_new (NULL);
		charset = g_io_channel_get_encoding (channel);
		if (charset)
			mimedir_profile_set_charset (profile, charset);
		newpos = mimedir_profile_parse (profile, string + pos, error);

		if (newpos < 0) {
			g_object_unref (G_OBJECT (profile));
			mimedir_vcal_free_list (list);
			list = NULL;
			break;
		}
		pos += newpos;

		vcal = mimedir_vcal_new_from_profile (profile, error);
		
		g_object_unref (G_OBJECT (profile));
		
		if (!vcal) {
			mimedir_vcal_free_list (list);
			list = NULL;
			break;
		}

		list = g_list_append (list, vcal);
	}

	g_free (string);

	return list;
}

/**
 * mimedir_vcal_free_list:
 * @list: list of vcals
 *
 * Frees a list of vcals as returned by mimedir_vcal_read_file()
 * or mimedir_vcal_read_channel().
 **/
void
mimedir_vcal_free_list (GList *list)
{
	GList *current;

	for (current = list; current != NULL; current = g_list_next (current))
		g_object_unref (G_OBJECT (current->data));

	g_list_free (list);
}

/**
 * mimedir_vcal_new:
 *
 * Creates a new (empty) vcal object.
 *
 * Return value: a new vcal object
 **/
MIMEDirVCal *
mimedir_vcal_new (void)
{
	MIMEDirVCal *vcal;

	vcal = g_object_new (MIMEDIR_TYPE_VCAL, NULL);

	return vcal;
}

/**
 * mimedir_vcal_new_from_profile:
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Create a new vcal object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vcal object or %NULL
 **/
MIMEDirVCal *
mimedir_vcal_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVCal *vcal;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vcal = g_object_new (MIMEDIR_TYPE_VCAL, NULL);

	if (!mimedir_vcal_read_from_profile (vcal, profile, error)) {
		g_object_unref (G_OBJECT (vcal));
		vcal = NULL;
	}

	return vcal;
}

/**
 * mimedir_vcal_read_from_profile:
 * @vcal: a vcal
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vcal object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcal_read_from_profile (MIMEDirVCal *vcal, MIMEDirProfile *profile, GError **error)
{
	MIMEDirVCalPriv *priv;
	GSList *attrs, *components;
	gchar *name;

	g_return_val_if_fail (vcal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	mimedir_vcal_reset (vcal);
	priv = vcal->priv;

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VCALENDAR") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCALENDAR");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	attrs = mimedir_profile_get_attribute_list (profile);

	for (; attrs != NULL; attrs = g_slist_next (attrs)) {
		MIMEDirAttribute *attr;

		attr = MIMEDIR_ATTRIBUTE (attrs->data);

		if (!mimedir_vcal_parse_attribute (vcal, attr, error))
			return FALSE;
	}

	/* Components */

	components = mimedir_profile_get_subprofiles (profile);

	for (; components != NULL; components = g_slist_next (components)) {
		MIMEDirProfile *profile;
		gchar *name;

		profile = MIMEDIR_PROFILE (components->data);

		g_object_get (G_OBJECT (profile), "name", &name, NULL);

		if (!g_ascii_strcasecmp (name, "VEVENT")) {
			MIMEDirVEvent *event;

			event = mimedir_vevent_new_from_profile (profile, error);
			if (!event) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, event);
		}

		else if (!g_ascii_strcasecmp (name, "VTODO")) {
			MIMEDirVTodo *todo;

			todo = mimedir_vtodo_new_from_profile (profile, error);
			if (!todo) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, todo);
		}

		else if (!g_ascii_strcasecmp (name, "VJOURNAL")) {
			MIMEDirVJournal *journal;

			journal = mimedir_vjournal_new_from_profile (profile, error);
			if (!journal) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, journal);
		}

		else if (!g_ascii_strcasecmp (name, "VFREEBUSY")) {
			MIMEDirVFreeBusy *fb;

			fb = mimedir_vfreebusy_new_from_profile (profile, error);
			if (!fb) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, fb);
		}

		else if (!g_ascii_strcasecmp (name, "VTIMEZONE")) {
			MIMEDirVTimeZone *tz;

			tz = mimedir_vtimezone_new_from_profile (profile, error);
			if (!tz) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, tz);
		}

		else if (!g_ascii_strcasecmp (name, "VALARM")) {
			MIMEDirVAlarm *alarm;

			alarm = mimedir_valarm_new_from_profile (profile, error);
			if (!alarm) {
				g_free (name);
				return FALSE;
			}

			priv->components = g_list_append (priv->components, alarm);
		}

		g_free (name);
	}

	return TRUE;
}

/**
 * mimedir_vcal_write_to_profile:
 * @vcal: a #MIMEDirVCal object
 *
 * Saves the iCal object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vcal_write_to_profile (MIMEDirVCal *vcal)
{
	MIMEDirVCalPriv *priv;
	MIMEDirProfile *profile;
	MIMEDirAttribute *attr;
	GList *i;

	g_return_val_if_fail (vcal != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), NULL);

	priv = vcal->priv;

	profile = mimedir_profile_new ("VCALENDAR");

	/*
	 * Calendar Properties (RFC 2445, Section 4.7)
	 */

	/* Calendar Scale (4.7.1) */

	/* Don't write default value: GREGORIAN. */

	/* Method (4.7.2) */

	if (priv->method) {
		attr = mimedir_attribute_new_with_name ("METHOD");
		mimedir_attribute_set_value_text (attr, priv->method);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Product Identifier (4.7.3) */

	attr = mimedir_attribute_new_with_name ("PRODID");
	mimedir_attribute_set_value (attr, "-//GNOME//NONSGML " PACKAGE_STRING "//EN");
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/* Version (4.7.4) */

	attr = mimedir_attribute_new_with_name ("VERSION");
	mimedir_attribute_set_value (attr, "2.0");
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/*
	 * Components
	 */

	for (i = priv->components; i != NULL; i = g_list_next (i)) {
		MIMEDirVComponent *comp;
		MIMEDirProfile *sub = NULL;

		g_assert (MIMEDIR_IS_VCOMPONENT (i->data));

		comp = MIMEDIR_VCOMPONENT (i->data);

		if (MIMEDIR_IS_VEVENT (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VEVENT");
		else if (MIMEDIR_IS_VTODO (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VTODO");
		else if (MIMEDIR_IS_VJOURNAL (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VJOURNAL");
		else if (MIMEDIR_IS_VFREEBUSY (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VFREEBUSY");
		else if (MIMEDIR_IS_VTIMEZONE (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VTIMEZONE");
		else if (MIMEDIR_IS_VALARM (comp))
			sub = mimedir_vcomponent_write_to_profile (comp, "VALARM");

		if (sub)
			mimedir_profile_append_subprofile (profile, sub);
	}

	return profile;
}

/**
 * mimedir_vcal_write_to_file:
 * @vcal: a #MIMEDirVCal object
 * @filename: file to write to
 * @error: error storage location or %NULL
 *
 * Saves the iCal object to a file. If an error occurs during the write,
 * @error will be set and %FALSE will be returned. Otherwise, %TRUE is
 * returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcal_write_to_file (MIMEDirVCal *vcal, const gchar *filename, GError **error)
{
	GIOChannel *channel;
	gboolean ret;

	g_return_val_if_fail (vcal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	channel = g_io_channel_new_file (filename, "w", error);
	if (!channel)
		return FALSE;

	ret = mimedir_vcal_write_to_channel (vcal, channel, error);
	
	g_io_channel_unref (channel);
	
	return ret;
}

/**
 * mimedir_vcal_write_to_channel:
 * @vcal: a #MIMEDirVCal object
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the iCal object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcal_write_to_channel (MIMEDirVCal *vcal, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vcal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vcal_write_to_profile (vcal);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vcal_write_to_string:
 * @vcal: a #MIMEDirVCal object
 *
 * Saves the iCal object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vcal_write_to_string (MIMEDirVCal *vcal)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vcal != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), FALSE);

	profile = mimedir_vcal_write_to_profile (vcal);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}

/**
 * mimedir_vcal_get_component_list:
 * @vcal: a vCalendar object
 *
 * Returns all components of this vCalendar. Free the returned list
 * with mimedir_vcal_free_component_list().
 *
 * Return value: a #GSList consisting of component objects
 **/
GSList *
mimedir_vcal_get_component_list (MIMEDirVCal *vcal)
{
	GList *item;
	GSList *list = NULL;

	g_return_val_if_fail (vcal != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), NULL);

	for (item = vcal->priv->components; item != NULL; item = g_list_next (item))
		list = g_slist_append (list, item->data);

	return list;
}

/**
 * mimedir_vcal_get_event_list:
 * @vcal: a vCalendar object
 *
 * Returns all vEvent components of this vCalendar. Free the returned list
 * with mimedir_vcal_free_component_list().
 *
 * Return value: a #GSList consisting of #MIMEDirVEvent objects
 **/
GSList *
mimedir_vcal_get_event_list (MIMEDirVCal *vcal)
{
	GList *item;
	GSList *list = NULL;

	g_return_val_if_fail (vcal != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), NULL);

	for (item = vcal->priv->components; item != NULL; item = g_list_next (item)) {
		if (MIMEDIR_IS_VEVENT (item->data))
		  {
			g_object_ref(item->data);
			list = g_slist_append (list, item->data);
		  }
	}

	return list;
}

/**
 * mimedir_vcal_get_todo_list:
 * @vcal: a vCalendar object
 *
 * Returns all vTodo components of this vCalendar. Free the returned list
 * with mimedir_vcal_free_component_list().
 *
 * Return value: a #GSList consisting of #MIMEDirVTodo objects
 **/
GSList *
mimedir_vcal_get_todo_list (MIMEDirVCal *vcal)
{
	GList *item;
	GSList *list = NULL;

	g_return_val_if_fail (vcal != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCAL (vcal), NULL);

	for (item = vcal->priv->components; item != NULL; item = g_list_next (item)) {
		if (MIMEDIR_IS_VTODO (item->data))
		  {
			g_object_ref (item->data);
			list = g_slist_append (list, item->data);
		  }
	}

	return list;
}

/**
 * mimedir_vcal_free_component_list:
 * @list: a #GSList
 *
 * Free a component list as returned by one of the get_*_list functions.
 **/
void
mimedir_vcal_free_component_list (GSList *list)
{
	g_slist_free (list);
}

/**
 * mimedir_vcal_add_component:
 * @vcal: a #MIMEDirVCal
 * @component: a #MIMEDirVComponent
 *
 * Adds a single component to the component list of @vcal.
 **/
void
mimedir_vcal_add_component (MIMEDirVCal *vcal, MIMEDirVComponent *component)
{
	g_return_if_fail (vcal != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (vcal));
	g_return_if_fail (component != NULL);
	g_return_if_fail (MIMEDIR_IS_VCOMPONENT (component));

	g_object_ref (G_OBJECT (component));

	vcal->priv->components = g_list_prepend (vcal->priv->components, component);
}

/**
 * mimedir_vcal_add_component_list:
 * @vcal: a #MIMEDirVCal
 * @list: a #GList of #MIMEDirVComponent objects
 *
 * Adds components to the component list of @vcal.
 **/
void
mimedir_vcal_add_component_list (MIMEDirVCal *vcal, GList *list)
{
	GList *item;

	g_return_if_fail (vcal != NULL);
	g_return_if_fail (MIMEDIR_IS_VCAL (vcal));

	for (item = list; item != NULL; item = g_list_next (item)) {
		MIMEDirVComponent *component;

		g_return_if_fail (item->data != NULL);
		g_return_if_fail (MIMEDIR_IS_VCOMPONENT (item->data));

		component = MIMEDIR_VCOMPONENT (item->data);

		mimedir_vcal_add_component (vcal, component);
	}
}
