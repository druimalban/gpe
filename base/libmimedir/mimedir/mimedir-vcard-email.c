/* RFC 2426 vCard MIME Directory Profile -- EMail Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-email.c 227 2005-09-11 16:09:51Z srittau $
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

#include <libintl.h>

#include "mimedir-marshal.h"
#include "mimedir-utils.h"
#include "mimedir-vcard-email.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcard_email_class_init		(MIMEDirVCardEMailClass	*klass);
static void	 mimedir_vcard_email_init		(MIMEDirVCardEMail		*email);
static void	 mimedir_vcard_email_dispose		(GObject			*object);
static void	 mimedir_vcard_email_set_property	(GObject		*object,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	 mimedir_vcard_email_get_property	(GObject		*object,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);

static void	 mimedir_vcard_email_clear		(MIMEDirVCardEMail	*email);


enum {
	PROP_ADDRESS = 1,
	PROP_TYPE,
	PROP_PREFERRED,
	PROP_HOME,
	PROP_WORK,
};

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};

struct _MIMEDirVCardEMailPriv {
	gchar *address;

	MIMEDirVCardEMailType type;

	gboolean preferred;
	gboolean home;
	gboolean work;
};

static GObjectClass *parent_class = NULL;

static int mimedir_vcard_email_signals[LAST_SIGNAL] = { 0 };

/*
 * Class and Object Management
 */

GType
mimedir_vcard_email_get_type (void)
{
	static GType mimedir_vcard_email_type = 0;

	if (!mimedir_vcard_email_type) {
		static const GTypeInfo mimedir_vcard_email_info = {
			sizeof (MIMEDirVCardEMailClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcard_email_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVCardEMail),
			5,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcard_email_init,
		};

		mimedir_vcard_email_type = g_type_register_static (G_TYPE_OBJECT,
								     "MIMEDirVCardEMail",
								     &mimedir_vcard_email_info,
								     0);
	}

	return mimedir_vcard_email_type;
}


static void
mimedir_vcard_email_class_init (MIMEDirVCardEMailClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_vcard_email_dispose;
	gobject_class->set_property = mimedir_vcard_email_set_property;
	gobject_class->get_property = mimedir_vcard_email_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcard_email_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVCardEMailClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_string ("address",
				     _("Address"),
				     _("The e-mail address"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ADDRESS, pspec);

	pspec = g_param_spec_int ("type",
				  _("Type"),
				  _("Type of this e-mail address"),
				  MIMEDIR_VCARD_EMAIL_TYPE_UNKNOWN,
				  MIMEDIR_VCARD_EMAIL_TYPE_X400,
				  MIMEDIR_VCARD_EMAIL_TYPE_INTERNET,
				  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TYPE, pspec);
	pspec = g_param_spec_boolean ("preferred",
				      _("Preferred"),
				      _("Whether this is the preferred e-mail address"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PREFERRED, pspec);
	pspec = g_param_spec_boolean ("home",
				      _("Home"),
				      _("Whether is a home e-mail address"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_HOME, pspec);
	pspec = g_param_spec_boolean ("work",
				      _("Work"),
				      _("Whether this is an e-mail address at work"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_WORK, pspec);
}


static void
mimedir_vcard_email_init (MIMEDirVCardEMail *email)
{
	g_return_if_fail (email != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (email));

	email->priv = g_new0 (MIMEDirVCardEMailPriv, 1);
	email->priv->type    = MIMEDIR_VCARD_EMAIL_TYPE_INTERNET;
}


static void
mimedir_vcard_email_dispose (GObject *object)
{
	MIMEDirVCardEMail     *email;
	MIMEDirVCardEMailPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (object));

	email = MIMEDIR_VCARD_EMAIL (object);

	priv = email->priv;

	if (priv)
		mimedir_vcard_email_clear (email);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_vcard_email_set_property_string (gchar **dest, const GValue *value)
{
	const gchar *string;

	string = g_value_get_string (value);
	g_free (*dest);
	*dest = g_strdup (string);
}


static void
mimedir_vcard_email_set_property (GObject	*object,
				  guint		 property_id,
				  const GValue	*value,
				  GParamSpec	*pspec)
{
	MIMEDirVCardEMailPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (object));

	priv = MIMEDIR_VCARD_EMAIL (object)->priv;

	switch (property_id) {
	case PROP_ADDRESS:
		mimedir_vcard_email_set_property_string (&priv->address, value);
		break;

	case PROP_TYPE: {
		MIMEDirVCardEMailType type;

		type = g_value_get_int (value);
		g_return_if_fail (type >= MIMEDIR_VCARD_EMAIL_TYPE_INTERNET && type <= MIMEDIR_VCARD_EMAIL_TYPE_X400);
		priv->type = type;
		break;
	}
	case PROP_PREFERRED:
		priv->preferred = g_value_get_boolean (value);
		break;
	case PROP_HOME:
		priv->home = g_value_get_boolean (value);
		break;
	case PROP_WORK:
		priv->work = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	g_signal_emit (object, mimedir_vcard_email_signals[SIGNAL_CHANGED], 0);
}


static void
mimedir_vcard_email_get_property (GObject	*object,
				  guint		 property_id,
				  GValue	*value,
				  GParamSpec	*pspec)
{
	MIMEDirVCardEMailPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (object));

	priv = MIMEDIR_VCARD_EMAIL (object)->priv;

	switch (property_id) {
	case PROP_ADDRESS:
		g_value_set_string (value, priv->address);
		break;

	case PROP_TYPE:
		g_value_set_int (value, priv->type);
		break;
	case PROP_PREFERRED:
		g_value_set_boolean (value, priv->preferred);
		break;
	case PROP_HOME:
		g_value_set_boolean (value, priv->home);
	case PROP_WORK:
		g_value_set_boolean (value, priv->work);

 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Private Methods
 */

static void
mimedir_vcard_email_clear (MIMEDirVCardEMail *email)
{
        MIMEDirVCardEMailPriv *priv;

        priv = email->priv;

	if (!priv)
		return;

	g_free (priv->address);

	g_free (priv);
	email->priv = NULL;
}


static void
mimedir_vcard_email_reset (MIMEDirVCardEMail *email)
{
	mimedir_vcard_email_clear (email);
	mimedir_vcard_email_init (email);
}

/*
 * Public Methods
 */

/**
 * mimedir_vcard_email_new:
 *
 * Create a new MIMEDirVCardEMail object.
 *
 * Return value: the MIMEDirVCardEMail object
 **/
MIMEDirVCardEMail *
mimedir_vcard_email_new (void)
{
	MIMEDirVCardEMail *email;

	email = g_object_new (MIMEDIR_TYPE_VCARD_EMAIL, NULL);

	return email;
}


/**
 * mimedir_vcard_email_new_from_attribute:
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Creates a new MIMEDirVCardEMail object, initializing it with data taken
 * from the attribute object.
 *
 * Return value: the MIMEDirVCardEMail object
 **/
MIMEDirVCardEMail *
mimedir_vcard_email_new_from_attribute (MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirVCardEMail *email;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	email = g_object_new (MIMEDIR_TYPE_VCARD_EMAIL, NULL);

	if (!mimedir_vcard_email_set_from_attribute (email, attribute, error)) {
		g_object_unref (G_OBJECT (email));
		email = NULL;
	}

	return email;
}

/**
 * mimedir_vcard_email_set_from_attribute:
 * @email: the object to manipulate
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Initializes the email object to data taken from the attribute object.
 * It is save to use this function, even if the object has been initialized
 * before. All old data will be lost in this case.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_email_set_from_attribute (MIMEDirVCardEMail *email, MIMEDirAttribute *attribute, GError **error)
{
	GError *err = NULL;
	MIMEDirVCardEMailPriv *priv;
	GSList *list, *l;
	const gchar *name;

	g_return_val_if_fail (email != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_EMAIL (email), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	name = mimedir_attribute_get_name (attribute);

	g_return_val_if_fail (g_ascii_strcasecmp (name, "EMAIL") == 0, FALSE);

	mimedir_vcard_email_reset (email);
	priv = email->priv;

	/*
	 * Parse content
	 */

	priv->address = mimedir_attribute_get_value_text (attribute, &err);
	if (!priv->address) {
		g_propagate_error (error, err);
		return FALSE;
	}

	/*
	 * Parse type attribute
	 */

	list = mimedir_attribute_get_parameter_values (attribute, "type");

	if (list) {
		for (l = list; l != NULL; l = g_slist_next (l)) {
			const gchar *type = (const gchar *) l->data;

			if (!g_ascii_strcasecmp (type, "internet"))
				priv->type = MIMEDIR_VCARD_EMAIL_TYPE_INTERNET;
			else if (!g_ascii_strcasecmp (type, "x400"))
				priv->type = MIMEDIR_VCARD_EMAIL_TYPE_X400;
			else if (!g_ascii_strcasecmp (type, "pref"))
				priv->preferred = TRUE;
			else if (!g_ascii_strcasecmp (type, "home")) // custom
				priv->home = TRUE;
			else if (!g_ascii_strcasecmp (type, "work")) // custom
				priv->work = TRUE;
			else
				g_printerr (_("The email field is of unknown type %s.\n"), type);
		}
	}

	mimedir_attribute_free_parameter_values (attribute, list);

	return TRUE;
}

/**
 * mimedir_vcard_email_save_to_attribute:
 * @email: an e-mail object
 *
 * Returns a new attribute that describes the e-mail address.
 *
 * Return value: a new attribute
 **/
MIMEDirAttribute *
mimedir_vcard_email_save_to_attribute (MIMEDirVCardEMail *email)
{
	MIMEDirVCardEMailPriv *priv;
	MIMEDirAttribute *attr;

	g_return_val_if_fail (email != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_EMAIL (email), NULL);

	priv = email->priv;

	attr = mimedir_attribute_new_with_name ("EMAIL");

	mimedir_attribute_set_value_text (attr, priv->address);

	if (priv->preferred)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "pref");
	if (priv->home)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "home");
	if (priv->work)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "work");

	switch (priv->type) {
	case MIMEDIR_VCARD_EMAIL_TYPE_INTERNET:
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "internet");
		break;
	case MIMEDIR_VCARD_EMAIL_TYPE_X400:
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "x400");
		break;
	default:
		break; /* dummy */
	}

	return attr;
}

/**
 * mimedir_vcard_email_get_as_string:
 * @email: the email object
 *
 * Returns the email address as a human-readable string. The returned string
 * should be freed with g_free().
 *
 * Return value: the email address as human-readable string
 **/
gchar *
mimedir_vcard_email_get_as_string (MIMEDirVCardEMail *email)
{
	g_return_val_if_fail (email != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_EMAIL (email), NULL);

	return g_strdup (email->priv->address);
}

/**
 * mimedir_vcard_email_get_type_string:
 * @email: the email object
 *
 * Returns the type(s) of the email as a human-readable string. It
 * should be freed with g_free().
 *
 * Return value: the type as human-readable string
 **/
gchar *
mimedir_vcard_email_get_type_string (MIMEDirVCardEMail *email)
{
	MIMEDirVCardEMailPriv *priv;
	GString *s;

	g_return_val_if_fail (email != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_EMAIL (email), NULL);

	priv = email->priv;

	s = g_string_new ("");

	if (priv->preferred) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("preferred"));
	}

	switch (priv->type) {
	case MIMEDIR_VCARD_EMAIL_TYPE_UNKNOWN:
		/* dummy */
		break;
	case MIMEDIR_VCARD_EMAIL_TYPE_INTERNET:
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("Internet"));
		break;
	case MIMEDIR_VCARD_EMAIL_TYPE_X400:
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("X.400"));
		break;
	}

	if (priv->home) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("home"));
	}

	if (priv->work) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("work"));
	}

	return g_string_free (s, FALSE);
}
