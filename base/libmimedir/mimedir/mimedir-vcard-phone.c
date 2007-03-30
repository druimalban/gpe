/* RFC 2426 vCard MIME Directory Profile -- Phone Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-phone.c 227 2005-09-11 16:09:51Z srittau $
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
#include "mimedir-vcard-phone.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcard_phone_class_init		(MIMEDirVCardPhoneClass	*klass);
static void	 mimedir_vcard_phone_init		(MIMEDirVCardPhone		*phone);
static void	 mimedir_vcard_phone_dispose		(GObject			*object);
static void	 mimedir_vcard_phone_set_property	(GObject		*object,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	 mimedir_vcard_phone_get_property	(GObject		*object,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);

static void	 mimedir_vcard_phone_clear		(MIMEDirVCardPhone	*phone);


enum {
	PROP_NUMBER = 1,

	PROP_HOME,
	PROP_WORK,
	PROP_VOICE,
	PROP_FAX,
	PROP_BBS,
	PROP_MODEM,
	PROP_PAGER,
	PROP_VIDEO,
	PROP_CELL,
	PROP_CAR,
	PROP_ISDN,
	PROP_PCS,
	PROP_PREFERRED,
	PROP_MESSAGE
};

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};

struct _MIMEDirVCardPhonePriv {
	gchar *number;

	gboolean preferred;
	gboolean home, work;
	gboolean voice, fax, video, pager, bbs, modem;
	gboolean car, cell;
	gboolean message, isdn, pcs;
};

static GObjectClass *parent_class = NULL;

static int mimedir_vcard_phone_signals[LAST_SIGNAL] = { 0 };

/*
 * Class and Object Management
 */

GType
mimedir_vcard_phone_get_type (void)
{
	static GType mimedir_vcard_phone_type = 0;

	if (!mimedir_vcard_phone_type) {
		static const GTypeInfo mimedir_vcard_phone_info = {
			sizeof (MIMEDirVCardPhoneClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcard_phone_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVCardPhone),
			5,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcard_phone_init,
		};

		mimedir_vcard_phone_type = g_type_register_static (G_TYPE_OBJECT,
								     "MIMEDirVCardPhone",
								     &mimedir_vcard_phone_info,
								     0);
	}

	return mimedir_vcard_phone_type;
}


static void
mimedir_vcard_phone_class_init (MIMEDirVCardPhoneClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_vcard_phone_dispose;
	gobject_class->set_property = mimedir_vcard_phone_set_property;
	gobject_class->get_property = mimedir_vcard_phone_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcard_phone_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVCardPhoneClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_string ("number",
				     _("Number"),
				     _("The telephone number (freeform string)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NUMBER, pspec);

	pspec = g_param_spec_boolean ("home",
				      _("Home"),
				      _("Whether this is a home phone number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_HOME, pspec);
	pspec = g_param_spec_boolean ("work",
				      _("Work"),
				      _("Whether this is a phone number at work"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_WORK, pspec);
	pspec = g_param_spec_boolean ("voice",
				      _("Voice"),
				      _("Whether this is a voice number"),
				      TRUE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_VOICE, pspec);
	pspec = g_param_spec_boolean ("fax",
				      _("Fax"),
				      _("Whether this is a facsimile number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FAX, pspec);
	pspec = g_param_spec_boolean ("bbs",
				      _("BBS"),
				      _("Whether this is a bulletin board system number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_BBS, pspec);
	pspec = g_param_spec_boolean ("modem",
				      _("Modem"),
				      _("Whether this is a modem number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_MODEM, pspec);
	pspec = g_param_spec_boolean ("pager",
				      _("Pager"),
				      _("Whether this is a pager number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PAGER, pspec);
	pspec = g_param_spec_boolean ("video",
				      _("Video"),
				      _("Whether this is a video phone number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_VIDEO, pspec);
	pspec = g_param_spec_boolean ("cell",
				      _("Cell"),
				      _("Whether this is a cell (mobile) phone number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CELL, pspec);
	pspec = g_param_spec_boolean ("car",
				      _("Car"),
				      _("Whether this is car phone number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CAR, pspec);
	pspec = g_param_spec_boolean ("isdn",
				      _("ISDN"),
				      _("Whether this is a ISDN number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ISDN, pspec);
	pspec = g_param_spec_boolean ("pcs",
				      _("PCS"),
				      _("Whether this is a PCS number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PCS, pspec);
	pspec = g_param_spec_boolean ("preferred",
				      _("Preferred"),
				      _("Whether this is the preferred number"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PREFERRED, pspec);
	pspec = g_param_spec_boolean ("message",
				      _("Message"),
				      _("Whether this number has a voice recorder"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_MESSAGE, pspec);
}


static void
mimedir_vcard_phone_init (MIMEDirVCardPhone *phone)
{
	g_return_if_fail (phone != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (phone));

	phone->priv = g_new0 (MIMEDirVCardPhonePriv, 1);
	phone->priv->voice = TRUE;
}


static void
mimedir_vcard_phone_dispose (GObject *object)
{
	MIMEDirVCardPhone     *phone;
	MIMEDirVCardPhonePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (object));

	phone = MIMEDIR_VCARD_PHONE (object);

	priv = phone->priv;

	if (priv)
		mimedir_vcard_phone_clear (phone);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_vcard_phone_set_property_string (gchar **dest, const GValue *value)
{
	const gchar *string;

	string = g_value_get_string (value);
	g_free (*dest);
	*dest = g_strdup (string);
}


static void
mimedir_vcard_phone_set_property (GObject	*object,
				  guint		 property_id,
				  const GValue	*value,
				  GParamSpec	*pspec)
{
	MIMEDirVCardPhonePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (object));

	priv = MIMEDIR_VCARD_PHONE (object)->priv;

	switch (property_id) {
	case PROP_NUMBER:
		mimedir_vcard_phone_set_property_string (&priv->number, value);
		break;

	case PROP_HOME:
		priv->home      = g_value_get_boolean (value);
		break;
	case PROP_WORK:
		priv->work      = g_value_get_boolean (value);
		break;
	case PROP_VOICE:
		priv->voice     = g_value_get_boolean (value);
		break;
	case PROP_FAX:
		priv->fax       = g_value_get_boolean (value);
		break;
	case PROP_BBS:
		priv->bbs       = g_value_get_boolean (value);
		break;
	case PROP_MODEM:
		priv->modem     = g_value_get_boolean (value);
		break;
	case PROP_PAGER:
		priv->pager     = g_value_get_boolean (value);
		break;
	case PROP_VIDEO:
		priv->video     = g_value_get_boolean (value);
		break;
	case PROP_CELL:
		priv->cell      = g_value_get_boolean (value);
		break;
	case PROP_CAR:
		priv->car       = g_value_get_boolean (value);
		break;
	case PROP_ISDN:
		priv->isdn      = g_value_get_boolean (value);
		break;
	case PROP_PCS:
		priv->pcs       = g_value_get_boolean (value);
		break;
	case PROP_PREFERRED:
		priv->preferred = g_value_get_boolean (value);
		break;
	case PROP_MESSAGE:
		priv->message   = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	g_signal_emit (object, mimedir_vcard_phone_signals[SIGNAL_CHANGED], 0);
}


static void
mimedir_vcard_phone_get_property (GObject	*object,
				  guint		 property_id,
				  GValue	*value,
				  GParamSpec	*pspec)
{
	MIMEDirVCardPhonePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (object));

	priv = MIMEDIR_VCARD_PHONE (object)->priv;

	switch (property_id) {
	case PROP_NUMBER:
		g_value_set_string (value, priv->number);
		break;

	case PROP_HOME:
		g_value_set_boolean (value, priv->home);
		break;
	case PROP_WORK:
		g_value_set_boolean (value, priv->work);
		break;
	case PROP_VOICE:
		g_value_set_boolean (value, priv->voice);
		break;
	case PROP_FAX:
		g_value_set_boolean (value, priv->fax);
		break;
	case PROP_BBS:
		g_value_set_boolean (value, priv->bbs);
		break;
	case PROP_MODEM:
		g_value_set_boolean (value, priv->modem);
		break;
	case PROP_PAGER:
		g_value_set_boolean (value, priv->pager);
		break;
	case PROP_VIDEO:
		g_value_set_boolean (value, priv->video);
		break;
	case PROP_CELL:
		g_value_set_boolean (value, priv->cell);
		break;
	case PROP_CAR:
		g_value_set_boolean (value, priv->car);
		break;
	case PROP_ISDN:
		g_value_set_boolean (value, priv->isdn);
		break;
	case PROP_PCS:
		g_value_set_boolean (value, priv->pcs);
		break;
	case PROP_PREFERRED:
		g_value_set_boolean (value, priv->preferred);
		break;
	case PROP_MESSAGE:
		g_value_set_boolean (value, priv->message);
		break;

 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Private Methods
 */

static void
mimedir_vcard_phone_clear (MIMEDirVCardPhone *phone)
{
        MIMEDirVCardPhonePriv *priv;

        priv = phone->priv;

	if (!priv)
		return;

	g_free (priv->number);

	g_free (priv);
	phone->priv = NULL;
}


static void
mimedir_vcard_phone_reset (MIMEDirVCardPhone *phone)
{
	mimedir_vcard_phone_clear (phone);
	mimedir_vcard_phone_init (phone);
}

/*
 * Public Methods
 */

/**
 * mimedir_vcard_phone_new:
 *
 * Create a new MIMEDirVCardPhone object.
 *
 * Return value: the MIMEDirVCardPhone object
 **/
MIMEDirVCardPhone *
mimedir_vcard_phone_new (void)
{
	MIMEDirVCardPhone *phone;

	phone = g_object_new (MIMEDIR_TYPE_VCARD_PHONE, NULL);

	return phone;
}


/**
 * mimedir_vcard_phone_new_from_attribute:
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Creates a new MIMEDirVCardPhone object, initializing it with data taken
 * from the attribute object.
 *
 * Return value: the MIMEDirVCardPhone object
 **/
MIMEDirVCardPhone *
mimedir_vcard_phone_new_from_attribute (MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirVCardPhone *phone;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	phone = g_object_new (MIMEDIR_TYPE_VCARD_PHONE, NULL);

	if (!mimedir_vcard_phone_set_from_attribute (phone, attribute, error)) {
		g_object_unref (G_OBJECT (phone));
		phone = NULL;
	}

	return phone;
}

/**
 * mimedir_vcard_phone_set_from_attribute:
 * @phone: the object to manipulate
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Initializes the phone object to data taken from the attribute object.
 * It is save to use this function, even if the object has been initialized
 * before. All old data will be lost in this case.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_phone_set_from_attribute (MIMEDirVCardPhone *phone, MIMEDirAttribute *attribute, GError **error)
{
	GError *err = NULL;
	MIMEDirVCardPhonePriv *priv;
	GSList *list, *l;
	const gchar *name;

	g_return_val_if_fail (phone != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_PHONE (phone), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	name = mimedir_attribute_get_name (attribute);

	g_return_val_if_fail (g_ascii_strcasecmp (name, "TEL") == 0, FALSE);

	mimedir_vcard_phone_reset (phone);
	priv = phone->priv;

	/*
	 * Parse content
	 */

	priv->number = mimedir_attribute_get_value_text (attribute, &err);
	if (!priv->number) {
		g_propagate_error (error, err);
		return FALSE;
	}

	/*
	 * Parse type attribute
	 */

	list = mimedir_attribute_get_parameter_values (attribute, "type");

	if (list) {
		phone->priv->voice = FALSE;

		for (l = list; l != NULL; l = g_slist_next (l)) {
			const gchar *type = (const gchar *) l->data;

			if (!g_ascii_strcasecmp (type, "pref"))
				priv->preferred = TRUE;
			else if (!g_ascii_strcasecmp (type, "home"))
				priv->home = TRUE;
			else if (!g_ascii_strcasecmp (type, "work"))
				priv->work = TRUE;
			else if (!g_ascii_strcasecmp (type, "msg"))
				priv->message = TRUE;
			else if (!g_ascii_strcasecmp (type, "voice"))
				priv->voice = TRUE;
			else if (!g_ascii_strcasecmp (type, "fax"))
				priv->fax = TRUE;
			else if (!g_ascii_strcasecmp (type, "cell"))
				priv->cell = TRUE;
			else if (!g_ascii_strcasecmp (type, "video"))
				priv->video = TRUE;
			else if (!g_ascii_strcasecmp (type, "pager"))
				priv->pager = TRUE;
			else if (!g_ascii_strcasecmp (type, "bbs"))
				priv->bbs = TRUE;
			else if (!g_ascii_strcasecmp (type, "modem"))
				priv->modem = TRUE;
			else if (!g_ascii_strcasecmp (type, "car"))
				priv->car = TRUE;
			else if (!g_ascii_strcasecmp (type, "isdn"))
				priv->isdn = TRUE;
			else if (!g_ascii_strcasecmp (type, "pcs"))
				priv->pcs = TRUE;
			else
				g_printerr (_("The phone field is of unknown type %s.\n"), type);
                }
        }

	mimedir_attribute_free_parameter_values (attribute, list);

	return TRUE;
}

/**
 * mimedir_vcard_phone_save_to_attribute:
 * @phone: a phone object
 *
 * Returns a new attribute that describes the phone number.
 *
 * Return value: a new attribute
 **/
MIMEDirAttribute *
mimedir_vcard_phone_save_to_attribute (MIMEDirVCardPhone *phone)
{
	MIMEDirVCardPhonePriv *priv;
	MIMEDirAttribute *attr;

	g_return_val_if_fail (phone != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_PHONE (phone), NULL);

	priv = phone->priv;

	attr = mimedir_attribute_new_with_name ("TEL");

	mimedir_attribute_set_value_text (attr, priv->number);

	if (priv->message)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "msg");
	if (priv->voice)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "voice");
	if (priv->fax)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "fax");
	if (priv->cell)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "cell");
	if (priv->home)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "home");
	if (priv->work)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "work");
	if (priv->preferred)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "pref");
	if (priv->video)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "video");
	if (priv->pager)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "pager");
	if (priv->bbs)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "bbs");
	if (priv->modem)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "modem");
	if (priv->car)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "car");
	if (priv->isdn)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "isdn");
	if (priv->pcs)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "pcs");

	return attr;
}

/**
 * mimedir_vcard_phone_get_as_string:
 * @phone: the phone object
 *
 * Returns the phone number as a human-readable string. The returned string
 * should be freed with g_free().
 *
 * Return value: the phone number as human-readable string
 **/
gchar *
mimedir_vcard_phone_get_as_string (MIMEDirVCardPhone *phone)
{
	g_return_val_if_fail (phone != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_PHONE (phone), NULL);

	return g_strdup (phone->priv->number);
}

/**
 * mimedir_vcard_phone_get_type_string:
 * @phone: the phone object
 *
 * Returns the type(s) of the phone as a human-readable string. It
 * should be freed with g_free().
 *
 * Return value: the type as human-readable string
 **/
gchar *
mimedir_vcard_phone_get_type_string (MIMEDirVCardPhone *phone)
{
	MIMEDirVCardPhonePriv *priv;
	GString *s;

	g_return_val_if_fail (phone != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_PHONE (phone), NULL);

	priv = phone->priv;

	s = g_string_new ("");

	if (priv->preferred) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("preferred"));
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
	if (priv->voice) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("voice"));
	}
	if (priv->fax) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("fax"));
	}
	if (priv->bbs) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("BBS"));
	}
	if (priv->modem) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("modem"));
	}
	if (priv->pager) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("pager"));
	}
	if (priv->video) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("video"));
	}
	if (priv->message) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("voice message"));
	}
	if (priv->cell) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("cell phone"));
	}
	if (priv->car) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("car phone"));
	}
	if (priv->isdn) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("ISDN"));
	}
	if (priv->pcs) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("PCS"));
	}

	return g_string_free (s, FALSE);
}
