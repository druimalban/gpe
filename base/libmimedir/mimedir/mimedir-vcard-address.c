/* RFC 2426 vCard MIME Directory Profile -- Address Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-address.c 227 2005-09-11 16:09:51Z srittau $
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

#include "mimedir-marshal.h"
#include "mimedir-utils.h"
#include "mimedir-vcard-address.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcard_address_class_init	(MIMEDirVCardAddressClass	*klass);
static void	 mimedir_vcard_address_init		(MIMEDirVCardAddress		*address);
static void	 mimedir_vcard_address_dispose	(GObject			*object);
static void	 mimedir_vcard_address_set_property	(GObject		*object,
							 guint			 property_id,
							 const GValue		*value,
							 GParamSpec		*pspec);
static void	 mimedir_vcard_address_get_property	(GObject		*object,
							 guint			 property_id,
							 GValue			*value,
							 GParamSpec		*pspec);

static void	 mimedir_vcard_address_clear		(MIMEDirVCardAddress	*address);


enum {
	PROP_TITLE = 1,

	PROP_FULL,
	PROP_POBOX,
	PROP_EXTENDED,
	PROP_STREET,
	PROP_LOCALITY,
	PROP_REGION,
	PROP_PCODE,
	PROP_COUNTRY,

	PROP_DOMESTIC,
	PROP_INTERNATIONAL,
	PROP_POSTAL,
	PROP_PARCEL,
	PROP_HOME,
	PROP_WORK,
	PROP_PREFERRED
};

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};

struct _MIMEDirVCardAddressPriv {
	gchar *pobox;
	gchar *extended;
	gchar *street;
	gchar *locality;
	gchar *region;
	gchar *pcode;
	gchar *country;

	gchar *full;

	gboolean domestic;
	gboolean international;
	gboolean postal;
	gboolean parcel;
	gboolean home;
	gboolean work;
	gboolean preferred;
};

static GObjectClass *parent_class = NULL;

static int mimedir_vcard_address_signals[LAST_SIGNAL] = { 0 };

/*
 * Class and Object Management
 */

GType
mimedir_vcard_address_get_type (void)
{
	static GType mimedir_vcard_address_type = 0;

	if (!mimedir_vcard_address_type) {
		static const GTypeInfo mimedir_vcard_address_info = {
			sizeof (MIMEDirVCardAddressClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcard_address_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVCardAddress),
			5,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcard_address_init,
		};

		mimedir_vcard_address_type = g_type_register_static (G_TYPE_OBJECT,
								     "MIMEDirVCardAddress",
								     &mimedir_vcard_address_info,
								     0);
	}

	return mimedir_vcard_address_type;
}


static void
mimedir_vcard_address_class_init (MIMEDirVCardAddressClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_vcard_address_dispose;
	gobject_class->set_property = mimedir_vcard_address_set_property;
	gobject_class->get_property = mimedir_vcard_address_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcard_address_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVCardAddressClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_string ("title",
				     _("Title"),
				     _("A one-line string with the most important address information"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_TITLE, pspec);

	pspec = g_param_spec_string ("full",
				     _("Freeform address"),
				     _("A freeform address string"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FULL, pspec);
	pspec = g_param_spec_string ("pobox",
				     _("Post office box"),
				     _("The addresses post office box"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_POBOX, pspec);
	pspec = g_param_spec_string ("extended",
				     _("Extended address"),
				     _("Extended address string"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_EXTENDED, pspec);
	pspec = g_param_spec_string ("street",
				     _("Street"),
				     _("Street address"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_STREET, pspec);
	pspec = g_param_spec_string ("locality",
				     _("Locality"),
				     _("Locality (e.g. city, town, etc.)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LOCALITY, pspec);
	pspec = g_param_spec_string ("region",
				     _("Region"),
				     _("Region (e.g. state)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_REGION, pspec);
	pspec = g_param_spec_string ("pcode",
				     _("Postal code"),
				     _("Postal code (freeform string)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PCODE, pspec);
	pspec = g_param_spec_string ("country",
				     _("Country"),
				     _("Country"),
				     NULL, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_COUNTRY, pspec);

	pspec = g_param_spec_boolean ("domestic",
				      _("Domestic"),
				      _("Whether this is a domestic address"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_DOMESTIC, pspec);
	pspec = g_param_spec_boolean ("international",
				      _("International"),
				      _("Whether this is an international address"),
				      TRUE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_INTERNATIONAL, pspec);
	pspec = g_param_spec_boolean ("postal",
				      _("Postal"),
				      _("Whether this is a postal address"),
				      TRUE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_POSTAL, pspec);
	pspec = g_param_spec_boolean ("parcel",
				      _("Parcel"),
				      _("Whether parcels can be received at this address"),
				      TRUE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PARCEL, pspec);
	pspec = g_param_spec_boolean ("home",
				      _("Home"),
				      _("Whether this is a home address"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_HOME, pspec);
	pspec = g_param_spec_boolean ("work",
				      _("Work"),
				      _("Whether this is a work address"),
				      TRUE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_WORK, pspec);
	pspec = g_param_spec_boolean ("preferred",
				      _("Preferred"),
				      _("Whether this is the preferred address"),
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PREFERRED, pspec);
}


static void
mimedir_vcard_address_init (MIMEDirVCardAddress *address)
{
	g_return_if_fail (address != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address));

	address->priv = g_new0 (MIMEDirVCardAddressPriv, 1);

	address->priv->international = TRUE;
	address->priv->postal        = TRUE;
	address->priv->parcel        = TRUE;
	address->priv->work          = TRUE;
}


static void
mimedir_vcard_address_dispose (GObject *object)
{
	MIMEDirVCardAddress     *address;
	MIMEDirVCardAddressPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (object));

	address = MIMEDIR_VCARD_ADDRESS (object);

	priv = address->priv;

	if (priv)
		mimedir_vcard_address_clear (address);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_vcard_address_set_property (GObject		*object,
				    guint		 property_id,
				    const GValue	*value,
				    GParamSpec		*pspec)
{
	MIMEDirVCardAddressPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (object));

	priv = MIMEDIR_VCARD_ADDRESS (object)->priv;

	switch (property_id) {
	case PROP_FULL:
		mimedir_utils_set_property_string (&priv->full, value);
		break;
	case PROP_POBOX:
		mimedir_utils_set_property_string (&priv->pobox, value);
		break;
	case PROP_EXTENDED:
		mimedir_utils_set_property_string (&priv->extended, value);
		break;
	case PROP_STREET:
		mimedir_utils_set_property_string (&priv->street, value);
		break;
	case PROP_LOCALITY:
		mimedir_utils_set_property_string (&priv->locality, value);
		break;
	case PROP_REGION:
		mimedir_utils_set_property_string (&priv->region, value);
		break;
	case PROP_PCODE:
		mimedir_utils_set_property_string (&priv->pcode, value);
		break;
	case PROP_COUNTRY:
		mimedir_utils_set_property_string (&priv->country, value);
		break;

	case PROP_DOMESTIC:
		priv->domestic      = g_value_get_boolean (value);
		break;
	case PROP_INTERNATIONAL:
		priv->international = g_value_get_boolean (value);
		break;
	case PROP_POSTAL:
		priv->postal        = g_value_get_boolean (value);
		break;
	case PROP_PARCEL:
		priv->parcel        = g_value_get_boolean (value);
		break;
	case PROP_HOME:
		priv->home          = g_value_get_boolean (value);
		break;
	case PROP_WORK:
		priv->work          = g_value_get_boolean (value);
		break;
	case PROP_PREFERRED:
		priv->preferred     = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	g_signal_emit (object, mimedir_vcard_address_signals[SIGNAL_CHANGED], 0);
}


static void
mimedir_vcard_address_get_property (GObject	*object,
				    guint	  property_id,
				    GValue	*value,
				    GParamSpec	*pspec)
{
	MIMEDirVCardAddressPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (object));

	priv = MIMEDIR_VCARD_ADDRESS (object)->priv;

	switch (property_id) {
	case PROP_TITLE: {
		gchar *title;
		title = mimedir_vcard_address_get_title (MIMEDIR_VCARD_ADDRESS (object));
		g_value_set_string (value, title);
		g_free (title);
		break;
	}

	case PROP_FULL:
		g_value_set_string (value, priv->full);
		break;
	case PROP_POBOX:
		g_value_set_string (value, priv->pobox);
		break;
	case PROP_EXTENDED:
		g_value_set_string (value, priv->extended);
		break;
	case PROP_STREET:
		g_value_set_string (value, priv->street);
		break;
	case PROP_LOCALITY:
		g_value_set_string (value, priv->locality);
		break;
	case PROP_REGION:
		g_value_set_string (value, priv->region);
		break;
	case PROP_PCODE:
		g_value_set_string (value, priv->pcode);
		break;
	case PROP_COUNTRY:
		g_value_set_string (value, priv->country);
		break;

	case PROP_DOMESTIC:
		g_value_set_boolean (value, priv->domestic);
		break;
	case PROP_INTERNATIONAL:
		g_value_set_boolean (value, priv->international);
		break;
	case PROP_POSTAL:
		g_value_set_boolean (value, priv->postal);
		break;
	case PROP_PARCEL:
		g_value_set_boolean (value, priv->parcel);
		break;
	case PROP_HOME:
		g_value_set_boolean (value, priv->home);
		break;
	case PROP_WORK:
		g_value_set_boolean (value, priv->work);
		break;
	case PROP_PREFERRED:
		g_value_set_boolean (value, priv->preferred);
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
mimedir_vcard_address_clear (MIMEDirVCardAddress *address)
{
        MIMEDirVCardAddressPriv *priv;

        priv = address->priv;

	if (!priv)
		return;

	g_free (priv->pobox);
	g_free (priv->extended);
	g_free (priv->street);
	g_free (priv->locality);
	g_free (priv->region);
	g_free (priv->pcode);
	g_free (priv->country);

	g_free (priv->full);

	g_free (priv);
	address->priv = NULL;
}


static void
mimedir_vcard_address_reset (MIMEDirVCardAddress *address)
{
	mimedir_vcard_address_clear (address);
	mimedir_vcard_address_init (address);
}


static gboolean
mimedir_vcard_address_set_adr_from_attribute (MIMEDirVCardAddress *address, MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirVCardAddressPriv *priv;
	GSList *list, *l;
	gchar *s;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = address->priv;

	/*
	 * Parse content
	 */

	list = mimedir_attribute_get_value_structured_text (attribute, error);
	if (!list)
		return FALSE;
	l = list;

	/* RFC 2426 requires the ADR type to have exactly seven components.
	 * For fault-tolerance reasons, we assume an empty string for each
	 * component that is not specified, and ignore all components
	 * after the seventh.
	 */

	/* Post office box */

	s = (gchar *) ((GSList *) l->data)->data;
	if (s && s[0] != '\0')
		priv->pobox = mimedir_utils_strcat_list ((GSList *) l->data, ",");

	l = g_slist_next (l);

	/* Extended address */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->extended = mimedir_utils_strcat_list ((GSList *) l->data, "\n");
	}
	l = g_slist_next (l);

	/* Street address */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->street = mimedir_utils_strcat_list ((GSList *) l->data, "\n");
	}
	l = g_slist_next (l);

	/* Locality */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->locality = mimedir_utils_strcat_list ((GSList *) l->data, ",");
	}
	l = g_slist_next (l);

	/* Region */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->region = mimedir_utils_strcat_list ((GSList *) l->data, ",");
	}
	l = g_slist_next (l);

	/* Postal code */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->pcode = mimedir_utils_strcat_list ((GSList *) l->data, ",");
	}
	l = g_slist_next (l);

	/* Country name */

	if (l) {
		s = (gchar *) ((GSList *) l->data)->data;
		if (s && s[0] != '\0')
			priv->country = mimedir_utils_strcat_list ((GSList *) l->data, ",");
	}
	l = g_slist_next (l);

	if (l)
		g_printerr (_("The %s attribute contains too many elements.\n"), "ADR");

	mimedir_attribute_free_structured_text_list (list);

	return TRUE;
}


static gboolean
mimedir_vcard_address_set_label_from_attribute (MIMEDirVCardAddress *address, MIMEDirAttribute *attribute, GError **error)
{
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	address->priv->full = mimedir_attribute_get_value_text (attribute, error);
	if (!address->priv->full)
		return FALSE;

	return TRUE;
}


static MIMEDirAttribute *
mimedir_vcard_address_save_adr_to_attribute (MIMEDirVCardAddress *address)
{
	MIMEDirVCardAddressPriv *priv;
	MIMEDirAttribute *attr;
	GSList *list = NULL, *list2;

	priv = address->priv;

	attr = mimedir_attribute_new_with_name ("ADR");

	list2 = g_slist_append (NULL, priv->pobox);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->extended);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->street);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->locality);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->region);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->pcode);
	list = g_slist_append (list, list2);
	list2 = g_slist_append (NULL, priv->country);
	list = g_slist_append (list, list2);

	mimedir_attribute_set_value_structured_text (attr, list);

	for (list2 = list; list2 != NULL; list2 = g_slist_next (list2))
		g_slist_free ((GSList *) list2->data);
	g_slist_free (list);

	return attr;
}


static MIMEDirAttribute *
mimedir_vcard_address_save_label_to_attribute (MIMEDirVCardAddress *address)
{
	MIMEDirAttribute *attr;

	attr = mimedir_attribute_new_with_name ("LABEL");

	mimedir_attribute_set_value_text (attr, address->priv->full);

	return attr;
}

/*
 * Public Methods
 */

/**
 * mimedir_vcard_address_new:
 *
 * Create a new MIMEDirVCardAddress object.
 *
 * Return value: the MIMEDirVCardAddress object
 **/
MIMEDirVCardAddress *
mimedir_vcard_address_new (void)
{
	MIMEDirVCardAddress *address;

	address = g_object_new (MIMEDIR_TYPE_VCARD_ADDRESS, NULL);

	return address;
}


/**
 * mimedir_vcard_address_new_from_attribute:
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Creates a new MIMEDirVCardAddress object, initializing it with data taken
 * from the attribute object.
 *
 * Return value: the MIMEDirVCardAddress object
 **/
MIMEDirVCardAddress *
mimedir_vcard_address_new_from_attribute (MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirVCardAddress *address;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	address = g_object_new (MIMEDIR_TYPE_VCARD_ADDRESS, NULL);

	if (!mimedir_vcard_address_set_from_attribute (address, attribute, error)) {
		g_object_unref (G_OBJECT (address));
		address = NULL;
	}

	return address;
}

/**
 * mimedir_vcard_address_set_from_attribute:
 * @address: the object to manipulate
 * @attribute: the object to take data from
 * @error: location to store the error occuring, or %NULL to ignore
 *
 * Initializes the address object to data taken from the attribute object.
 * It is save to use this function, even if the object has been initialized
 * before. All old data will be lost in this case.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_address_set_from_attribute (MIMEDirVCardAddress *address, MIMEDirAttribute *attribute, GError **error)
{
	GError *err = NULL;
	MIMEDirVCardAddressPriv *priv;
	GSList *list, *l;
	const gchar *name;

	g_return_val_if_fail (address != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	mimedir_vcard_address_reset (address);

	name = mimedir_attribute_get_name (attribute);

	/*
	 * Parse content
	 */

	if (!g_ascii_strcasecmp (name, "ADR"))
		mimedir_vcard_address_set_adr_from_attribute (address, attribute, &err);
	else if (!g_ascii_strcasecmp (name, "LABEL"))
		mimedir_vcard_address_set_label_from_attribute (address, attribute, &err);
	else {
		g_critical ("wrong attribute type %s", name);
		return FALSE;
	}

	if (err) {
		g_propagate_error (error, err);
		return FALSE;
	}

	/*
	 * Parse type attribute
	 */

	priv = address->priv;

	list = mimedir_attribute_get_parameter_values (attribute, "type");

	if (list) {
		priv->international = FALSE;
		priv->postal        = FALSE;
		priv->parcel        = FALSE;
		priv->work          = FALSE;

		for (l = list; l != NULL; l = g_slist_next (l)) {
			const gchar *type = (const gchar *) l->data;

			if (!g_ascii_strcasecmp (type, "dom"))
				priv->domestic = TRUE;
			else if (!g_ascii_strcasecmp (type, "intl"))
				priv->international = TRUE;
			else if (!g_ascii_strcasecmp (type, "postal"))
				priv->postal = TRUE;
			else if (!g_ascii_strcasecmp (type, "parcel"))
				priv->parcel = TRUE;
			else if (!g_ascii_strcasecmp (type, "home"))
				priv->home = TRUE;
			else if (!g_ascii_strcasecmp (type, "work"))
				priv->work = TRUE;
			else if (!g_ascii_strcasecmp (type, "pref"))
				priv->preferred = TRUE;
			else
				g_printerr (_("The address field is of unknown type %s.\n"), type);
		}
	}

	mimedir_attribute_free_parameter_values (attribute, list);

	return TRUE;
}

/**
 * mimedir_vcard_address_save_to_attribute:
 * @address: an address object
 *
 * Returns a new attribute that describes the address.
 *
 * Return value: a new attribute
 **/
MIMEDirAttribute *
mimedir_vcard_address_save_to_attribute (MIMEDirVCardAddress *address)
{
	MIMEDirVCardAddressPriv *priv;
	MIMEDirAttribute *attr;

	g_return_val_if_fail (address != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address), NULL);

	priv = address->priv;

	if (priv->full)
		attr = mimedir_vcard_address_save_label_to_attribute (address);
	else
		attr = mimedir_vcard_address_save_adr_to_attribute (address);

	if (priv->domestic)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "dom");
	if (priv->international)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "intl");
	if (priv->postal)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "postal");
	if (priv->parcel)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "parcel");
	if (priv->home)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "home");
	if (priv->work)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "work");
	if (priv->preferred)
		mimedir_attribute_append_parameter_simple (attr, "TYPE", "pref");

	return attr;
}

/**
 * mimedir_vcard_address_get_as_string:
 * @address: an address object
 *
 * Returns the address as a human-readable string. The returned string
 * should be freed with g_free().
 *
 * Return value: the address as human-readable string
 **/
gchar *
mimedir_vcard_address_get_as_string (MIMEDirVCardAddress *address)
{
	MIMEDirVCardAddressPriv *priv;
	GString *s;
	gchar *locality = NULL;

	g_return_val_if_fail (address != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address), NULL);

	priv = address->priv;

	s = g_string_new ("");

	if (priv->full)
		return g_strdup (priv->full);

	if (priv->pobox) {
		g_string_append (s, priv->pobox);
		g_string_append_c (s, '\n');
	}
	if (priv->street) {
		g_string_append (s, priv->street);
		g_string_append_c (s, '\n');
	}
	if (priv->extended) {
		g_string_append (s, priv->extended);
		g_string_append_c (s, '\n');
	}
	if (priv->locality) {
		if (priv->pcode)
			/* Translators: pcode city */
			locality = g_strdup_printf (_("%s %s"), priv->pcode, priv->locality);
		else
			locality = g_strdup (priv->locality);
	}
	if (priv->region) {
		if (locality)
			/* Translators: city, region */
			g_string_append_printf (s, _("%s, %s"), locality, priv->region);
		else if (priv->pcode)
			g_string_append_printf (s, _("%s %s"), priv->pcode, priv->region);
		else
			g_string_append (s, priv->region);
		g_string_append_c (s, '\n');
	} else if (locality) {
		g_string_append (s, locality);
		g_string_append_c (s, '\n');
	}
	if (priv->country) {
		g_string_append (s, priv->country);
		g_string_append_c (s, '\n');
	}

	return g_string_free (s, FALSE);
}

/**
 * mimedir_vcard_address_get_type_string:
 * @address: the address object
 *
 * Returns the type(s) of the address as a human-readable string. It
 * should be freed with g_free().
 *
 * Return value: the type as human-readable string
 **/
gchar *
mimedir_vcard_address_get_type_string (MIMEDirVCardAddress *address)
{
	MIMEDirVCardAddressPriv *priv;
	GString *s;

	g_return_val_if_fail (address != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address), NULL);

	priv = address->priv;

	s = g_string_new ("");

	if (priv->preferred) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("preferred"));
	}
	if (priv->domestic) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("domestic"));
	}
	if (priv->international) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("international"));
	}
	if (priv->postal) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("postal"));
	}
	if (priv->parcel) {
		if (s->str[0] != '\0')
			g_string_append (s, ", ");
		g_string_append (s, _("parcel"));
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

/**
 * mimedir_vcard_address_get_title:
 * @address: a vCard address
 *
 * Returns a one-line string with the most important address information.
 * The returned string should be freed with g_free().
 *
 * Return value: title string
 **/
gchar *
mimedir_vcard_address_get_title (MIMEDirVCardAddress *address)
{
	MIMEDirVCardAddressPriv *priv;
	const gchar *loc = NULL;
	const gchar *add = NULL;

	priv = address->priv;

	if (priv->full && priv->full[0] != '\0') {
		gchar *pos;
		gsize size;

		pos = strchr (priv->full, '\n');
		if (pos)
			size = pos - priv->full;
		else
			size = strlen (priv->full);
		return g_strndup (priv->full, size);
	}

	if (priv->locality && priv->locality[0] != '\0')
		loc = priv->locality;
	else if (priv->region && priv->region[0] != '\0')
		loc = priv->region;
	else if (priv->country && priv->country[0] != '\0')
		loc = priv->country;

	if (priv->street && priv->street[0] != '\0')
		add = priv->street;
	else if (priv->pobox && priv->pobox[0] != '\0')
		add = priv->pobox;
	else if (priv->extended && priv->extended[0] != '\0')
		add = priv->extended;

	if (loc && add)
		/* Translators: Short address format: "locality/street" */
		return g_strdup_printf (_("%s/%s"), loc, add);
	else if (loc)
		return g_strdup (loc);
	else if (add)
		return g_strdup (add);
	else
		return g_strdup (_("<unknown>"));
}
