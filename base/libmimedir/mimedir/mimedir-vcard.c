/* RFC 2426 vCard MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard.c 227 2005-09-11 16:09:51Z srittau $
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

#include <mimedir/mimedir-datetime.h>

#include "mimedir-datetime.h"
#include "mimedir-marshal.h"
#include "mimedir-profile.h"
#include "mimedir-utils.h"
#include "mimedir-vcard.h"
#include "mimedir-vcard-address.h"
#include "mimedir-vcard-email.h"
#include "mimedir-vcard-phone.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_vcard_class_init	(MIMEDirVCardClass	*klass);
static void	 mimedir_vcard_init		(MIMEDirVCard		*vcard);
static void	 mimedir_vcard_dispose		(GObject		*object);
static void	 mimedir_vcard_set_property	(GObject		*object,
						 guint			 property_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void	 mimedir_vcard_get_property	(GObject		*object,
						 guint			 property_id,
						 GValue			*value,
						 GParamSpec		*pspec);

static void	 mimedir_vcard_clear		(MIMEDirVCard		*vcard);
static void	 mimedir_vcard_changed		(MIMEDirVCard		*vcard);

static void	 mimedir_vcard_append_address_list
						(MIMEDirVCard		*vcard,
						 GSList			*list);
static void	 mimedir_vcard_free_address_list
						(MIMEDirVCard		*vcard);
static void	 mimedir_vcard_append_phone_list
						(MIMEDirVCard		*vcard,
						 GSList			*list);
static void	 mimedir_vcard_free_phone_list	(MIMEDirVCard		*vcard);
static void	 mimedir_vcard_append_email_list
						(MIMEDirVCard		*vcard,
						 GSList			*list);
static void	 mimedir_vcard_free_email_list	(MIMEDirVCard		*vcard);

static GSList	*mimedir_vcard_strsplit		(const gchar *string,
						 const gchar *separator);
static gchar	*mimedir_vcard_strjoin		(const gchar *separator,
						 GSList *list);


enum {
	PROP_NAME = 1,
	PROP_FAMILYNAME,
	PROP_GIVENNAME,
	PROP_MIDDLENAME,
	PROP_PREFIX,
	PROP_SUFFIX,
	PROP_NICKNAME_LIST,
	PROP_NICKNAME,
	PROP_PHOTO,
	PROP_PHOTO_URI,
	PROP_BIRTHDAY,

	PROP_ADDRESS_LIST,
	PROP_ADDRESS,

	PROP_PHONE_LIST,
	PROP_PHONE,
	PROP_EMAIL_LIST,
	PROP_EMAIL,
	PROP_MAILER,

	PROP_TIMEZONE,
	PROP_TIMEZONE_STRING,
	PROP_LATITUDE,
	PROP_LONGITUDE,

	PROP_JOBTITLE,
	PROP_JOBROLE,
	PROP_LOGO,
	PROP_LOGO_URI,
	PROP_AGENT,
	PROP_AGENT_URI,
	PROP_AGENT_STRING,
	PROP_ORGANIZATION_LIST,
	PROP_ORGANIZATION,

	PROP_CATEGORY_LIST,
	PROP_CATEGORIES,
	PROP_NOTE,
	PROP_PRODID,
	PROP_REVISION,
	PROP_SORT_STRING,
	PROP_SOUND,
	PROP_SOUND_URI,
	PROP_UID,
	PROP_URL,

	PROP_CLASS,
	PROP_KEY_LIST,
	PROP_PUBKEY,
	PROP_KEYTYPE
};

enum {
	SIGNAL_CHANGED,
	SIGNAL_LAST
};

struct _MIMEDirVCardPriv {
	gboolean changed;

	/* Generic Types (Section 2.1) */

	gchar	*name;			/* 2.1.2: NAME */
	gchar	*source;		/* 2.1.4: SOURCE */

	/* Identification Types (Section 3.1) */

	gchar	*fn;			/* 3.1.1: FN */
	gchar	*familyname;		/* 3.1.2: N */
	gchar	*givenname;
	gchar	*middlename;
	gchar	*prefix;
	gchar	*suffix;
	GSList	*nickname;		/* 3.1.3: NICKNAME */
	GString	*photo;			/* 3.1.4: PHOTO */
	gchar	*photo_uri;
	MIMEDirDateTime
		*birthday;		/* 3.1.5: BDAY */

	/* Delivery Addressing Types (Section 3.2) */

	GSList	*addresses;		/* 3.2.1: ADR, 3.2.2: LABEL */
	GSList	*address_signals;

	/* Telecommunications Addressing Types (Section 3.3) */

	GSList	*phone;			/* 3.3.1: TEL */
	GSList	*phone_signals;
	GSList	*email;			/* 3.3.2: EMAIL */
	GSList	*email_signals;
	gchar	*mailer;		/* 3.3.3: MAILER */

	/* Geographical Types (Section 3.4) */

	gint	 timezone;		/* 3.4.1: TZ */
	gchar	*timezone_string;
	gdouble	 latitude, longitude;	/* 3.4.2: GEO */

	/* Organizational Types (Section 3.5) */

	gchar	*jobtitle;		/* 3.5.1: TITLE */
	gchar	*jobrole;		/* 3.5.2: ROLE */
	GString	*logo;			/* 3.5.3: LOGO */
	gchar	*logo_uri;
	MIMEDirVCard			/* 3.5.4: AGENT */
		*agent;
	gchar	*agent_string;
	gchar	*agent_uri;
	GSList	*organization;		/* 3.5.5: ORG */

	/* Explanatory Types (Section 3.6) */

	GSList	*categories;		/* 3.6.1: CATEGORIES */
	gchar	*note;			/* 3.6.2: NOTE */
	gchar	*prodid;		/* 3.6.3: PRODID */
	MIMEDirDateTime
		*revision;		/* 3.6.4: REV */
	gchar	*sort_string;		/* 3.6.5: SORT-STRING */
	GString	*sound;			/* 3.6.6: SOUND */
	gchar	*sound_uri;
	gchar	*uid;			/* 3.6.7: UID */
	gchar	*url;			/* 3.6.8: URL */
	/* unused */			/* 3.6.9: VERSION */

	/* Security Types (Section 3.7) */

	gchar	*klass;			/* 3.7.1: CLASS */
	GList	*keys;			/* 3.7.2: KEY */
};

static MIMEDirProfileClass *parent_class = NULL;

static gint mimedir_vcard_signals[SIGNAL_LAST] = { 0 };

/*
 * Utility Functions
 */

static GSList *
mimedir_vcard_copy_string_list (GSList *list)
{
	GSList *newlist = NULL;

	for (; list != NULL; list = g_slist_next (list)) {
		gchar *s;
		s = g_strdup ((const gchar *) list->data);
		newlist = g_slist_append (newlist, s);
	}

	return newlist;
}

/*
 * Class and Object Management
 */

GType
mimedir_vcard_get_type (void)
{
	static GType mimedir_vcard_type = 0;

	if (!mimedir_vcard_type) {
		static const GTypeInfo mimedir_vcard_info = {
			sizeof (MIMEDirVCardClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_vcard_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirVCard),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_vcard_init,
		};

		mimedir_vcard_type = g_type_register_static (G_TYPE_OBJECT,
							     "MIMEDirVCard",
							     &mimedir_vcard_info,
							     0);
	}

	return mimedir_vcard_type;
}


static void
mimedir_vcard_class_init (MIMEDirVCardClass *klass)
{
	GObjectClass *gobject_class;
	GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose  = mimedir_vcard_dispose;
	gobject_class->set_property = mimedir_vcard_set_property;
	gobject_class->get_property = mimedir_vcard_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Signals */

	mimedir_vcard_signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MIMEDirVCardClass, changed),
			      NULL, NULL,
			      mimedir_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	/* Properties */

	pspec = g_param_spec_string ("name",
				     _("Name"),
				     _("The spelled out name full name of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NAME, pspec);
	pspec = g_param_spec_string ("familyname",
				     _("Family name"),
				     _("The family name of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_FAMILYNAME, pspec);
	pspec = g_param_spec_string ("givenname",
				     _("Given name"),
				     _("The given name of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_GIVENNAME, pspec);
	pspec = g_param_spec_string ("middlename",
				     _("Middle name"),
				     _("The middle name or middle initials of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_MIDDLENAME, pspec);
	pspec = g_param_spec_string ("prefix",
				     _("Prefix"),
				     _("The name prefix (e.g. honorific prefix) of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PREFIX, pspec);
	pspec = g_param_spec_string ("suffix",
				     _("Suffix"),
				     _("The name suffix of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SUFFIX, pspec);
	pspec = g_param_spec_pointer ("nickname-list",
				      _("Nickname list"),
				      _("List of all nick names. This is a GList *, where the elements are of type const gchar *"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NICKNAME_LIST, pspec);
	pspec = g_param_spec_string ("nickname",
				     _("Nick name"),
				     _("The first nick name"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NICKNAME, pspec);
	pspec = g_param_spec_pointer ("photo", /* FIXME: this is useless */
				      _("Photo"),
				      _("Pointer to a byte stream, representing an image of the person"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PHOTO, pspec);
	pspec = g_param_spec_string ("photo-uri",
				     _("Photo URI"),
				     _("URI to an image of this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PHOTO_URI, pspec);
	pspec = g_param_spec_object ("birthday",
				     _("Birthday"),
				     _("The person's birthday"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_BIRTHDAY, pspec);

	pspec = g_param_spec_pointer ("address-list",
				      _("Address list"),
				      _("List of all addresses. This is a GList *, where the elements are of type MIMEDirAddress *"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ADDRESS_LIST, pspec);
	pspec = g_param_spec_string ("address",
				     _("Address"),
				     _("The preferred address"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_ADDRESS, pspec);

	pspec = g_param_spec_pointer ("phone-list",
				      _("Phone number list"),
				      _("List of all phone numbers. This is a GList *, where the elements are of type MIMEDirPhone *"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_PHONE_LIST, pspec);
	pspec = g_param_spec_string ("phone",
				     _("Phone number"),
				     _("The preferred phone number"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PHONE, pspec);
	pspec = g_param_spec_pointer ("email-list",
				      _("E-mail address list"),
				      _("List of all e-mail addresses. This is a GList *, where the elements are of type MIMEDirEmail *"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_EMAIL_LIST, pspec);
	pspec = g_param_spec_string ("email",
				     _("E-mail address"),
				     _("The preferred e-mail address"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_EMAIL, pspec);
	pspec = g_param_spec_string ("mailer",
				     _("Mailer"),
				     _("The mailing program used by this person"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_MAILER, pspec);

	pspec = g_param_spec_int ("timezone",
				  _("Time zone"),
				  _("The time zone this person lives in"),
				  MIMEDIR_VCARD_TIMEZONE_MIN,
				  MIMEDIR_VCARD_TIMEZONE_MAX,
				  0,
				  G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TIMEZONE, pspec);
	pspec = g_param_spec_string ("timezone-string",
				     _("Time zone string"),
				     _("The time zone this person lives in (deprecated string representation)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_TIMEZONE_STRING, pspec);
	pspec = g_param_spec_double ("latitude",
				     _("Latitude"),
				     _("Latitude of this person's residence"),
				     MIMEDIR_VCARD_LATITUDE_MIN,
				     MIMEDIR_VCARD_LATITUDE_MAX,
				     0.0,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LATITUDE, pspec);
	pspec = g_param_spec_double ("longitude",
				     _("Longitude"),
				     _("Longitude of this person's residence"),
				     MIMEDIR_VCARD_LONGITUDE_MIN,
				     MIMEDIR_VCARD_LONGITUDE_MAX,
				     0.0,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LONGITUDE, pspec);

	pspec = g_param_spec_string ("jobtitle",
				     _("Job title"),
				     _("The person's job title"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_JOBTITLE, pspec);
	pspec = g_param_spec_string ("role",
				     _("Role"),
				     _("The person's job role"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_JOBROLE, pspec);
	pspec = g_param_spec_pointer ("logo", /* FIXME: useless */
				      _("Logo"),
				      _("Pointer to a byte stream, representing the logo of the person's company"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LOGO, pspec);
	pspec = g_param_spec_string ("logo-uri",
				     _("Logo URI"),
				     _("URI to the logo of this person's company"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_LOGO_URI, pspec);
	pspec = g_param_spec_object ("agent",
				     _("Agent"),
				     _("A person, acting for the person described in this card"),
				     MIMEDIR_TYPE_VCARD,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_AGENT, pspec);
	pspec = g_param_spec_string ("agent-uri",
				     _("Agent URI"),
				     _("URI to the description of a person, acting for the person described in this card"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_AGENT_URI, pspec);
	pspec = g_param_spec_string ("agent-string",
				     _("Agent string"),
				     _("A person, acting for the person described in this card (deprecated string representation)"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_AGENT_STRING, pspec);
	pspec = g_param_spec_pointer ("organization-list",
				      _("Organization list"),
				      _("List of all organization this person belongs to. This is a GList *, where the elements are gchar *'s"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ORGANIZATION_LIST, pspec);
	pspec = g_param_spec_string ("organization",
				     _("Organization"),
				     _("The person's first organization"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_ORGANIZATION, pspec);

	pspec = g_param_spec_pointer ("category-list",
				      _("Category list"),
				      _("List of all categories of this person. This is a GList *, where the elements are gchar *'s"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CATEGORY_LIST, pspec);
	pspec = g_param_spec_string ("categories",
				     _("Categories"),
				     _("String representation of this person's categories"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CATEGORIES, pspec);
	pspec = g_param_spec_string ("note",
				     _("Note"),
				     _("Additional comments"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NOTE, pspec);
	pspec = g_param_spec_string ("prodid",
				     _("Product identifier"),
				     _("The product identifier of the vCard. In case of newly generated objects, this is LibMIMEDir's product identifier"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PRODID, pspec);
	pspec = g_param_spec_object ("revision",
				     _("Revision"),
				     _("Card's revision"),
				     MIMEDIR_TYPE_DATETIME,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_REVISION, pspec);
	pspec = g_param_spec_string ("sort-string",
				     _("Sort string"),
				     _("String that defines this card's sort order"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SORT_STRING, pspec);
	pspec = g_param_spec_pointer ("sound", /* FIXME: useless */
				      _("Sound"),
				      _("Pointer to a byte stream, representing a sound file"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SOUND, pspec);
	pspec = g_param_spec_string ("sound-uri",
				     _("Sound URI"),
				     _("URI to a sound file"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_SOUND_URI, pspec);
	pspec = g_param_spec_string ("uid",
				     _("Unique Identifier"),
				     _("A unique identifier for this card"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_UID, pspec);
	pspec = g_param_spec_string ("url",
				     _("URL"),
				     _("An alternate URL for this resource"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_URL, pspec);

	pspec = g_param_spec_string ("class",
				     _("Classification"),
				     _("The security classfication of this card"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_CLASS, pspec);
	pspec = g_param_spec_pointer ("key-list",
				      _("Public key list"),
				      _("List of this person's public keys. This is a GList *, where the elements are MIMEDirVCardKeys"),
				      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_KEY_LIST, pspec);
	pspec = g_param_spec_string ("pubkey",
				     _("Public key"),
				     _("This person's first public key"),
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_PUBKEY, pspec);
	pspec = g_param_spec_int ("keytype",
				  _("Key type"),
				  _("Type of the first public key"),
				  MIMEDIR_VCARD_KEY_UNKNOWN, MIMEDIR_VCARD_KEY_PGP, MIMEDIR_VCARD_KEY_UNKNOWN, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class, PROP_KEYTYPE, pspec);
}


static void
mimedir_vcard_init (MIMEDirVCard *vcard)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));

	vcard->priv = g_new0 (MIMEDirVCardPriv, 1);

	vcard->priv->changed   = FALSE;
	vcard->priv->timezone  = G_MAXINT;
	vcard->priv->latitude  = G_MAXDOUBLE;
	vcard->priv->longitude = G_MAXDOUBLE;
}


static void
mimedir_vcard_dispose (GObject *object)
{
	MIMEDirVCard     *vcard;
	MIMEDirVCardPriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (object));

	vcard = MIMEDIR_VCARD (object);

	priv = vcard->priv;

	if (priv)
		mimedir_vcard_clear (vcard);

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_vcard_set_property (GObject		*object,
			    guint		 property_id,
			    const GValue	*value,
			    GParamSpec		*pspec)
{
	MIMEDirVCard *vcard;
	MIMEDirVCardPriv *priv;

	GString *str;
	GList *node;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (object));

	vcard = MIMEDIR_VCARD (object);
	priv = vcard->priv;

	switch (property_id) {
	case PROP_NAME:
		mimedir_utils_set_property_string (&priv->fn, value);
		break;
	case PROP_FAMILYNAME:
		mimedir_utils_set_property_string (&priv->familyname, value);
		break;
	case PROP_GIVENNAME:
		mimedir_utils_set_property_string (&priv->givenname, value);
		break;
	case PROP_MIDDLENAME:
		mimedir_utils_set_property_string (&priv->middlename, value);
		break;
	case PROP_PREFIX:
		mimedir_utils_set_property_string (&priv->prefix, value);
		break;
	case PROP_SUFFIX:
		mimedir_utils_set_property_string (&priv->suffix, value);
		break;
	case PROP_NICKNAME_LIST:
		mimedir_utils_free_string_slist (priv->nickname);
		priv->nickname = mimedir_vcard_copy_string_list ((GSList *) g_value_get_pointer (value));
		break;
	case PROP_NICKNAME:
	  /* Setting the Nickname attribute replaces the list with the comma-separated nicknames */
		mimedir_utils_free_string_slist (priv->nickname);
		priv->nickname = mimedir_vcard_strsplit (g_value_get_string (value), ",");
		break;
	case PROP_PHOTO:
		g_string_free (priv->photo, TRUE);
		g_free (priv->photo_uri);
		priv->photo = NULL;
		priv->photo_uri = NULL;
		str = g_value_get_pointer (value);
		if (str)
			priv->photo = g_string_new_len (str->str, str->len);
		break;
	case PROP_PHOTO_URI:
		g_string_free (priv->photo, TRUE);
		g_free (priv->photo_uri);
		priv->photo = NULL;
		priv->photo_uri = NULL;
		mimedir_utils_set_property_string (&priv->photo_uri, value);
		break;
	case PROP_BIRTHDAY:
		if (priv->birthday)
			g_object_unref (G_OBJECT (priv->birthday));
		priv->birthday = g_value_get_object (value);
		if (priv->birthday)
			g_object_ref (G_OBJECT (priv->birthday));
		break;

	case PROP_ADDRESS_LIST:
		mimedir_vcard_free_address_list (vcard);
		mimedir_vcard_append_address_list (vcard, (GSList *) g_value_get_pointer (value));
		break;

	case PROP_PHONE_LIST:
		mimedir_vcard_free_phone_list (vcard);
		mimedir_vcard_append_phone_list (vcard, (GSList *) g_value_get_pointer (value));
		break;
	case PROP_EMAIL_LIST:
		mimedir_vcard_free_email_list (vcard);
		mimedir_vcard_append_email_list (vcard, (GSList *) g_value_get_pointer (value));
		break;
	case PROP_MAILER:
		mimedir_utils_set_property_string (&priv->mailer, value);
		break;

	case PROP_TIMEZONE:
		priv->timezone = g_value_get_int (value);
		if (priv->timezone > MIMEDIR_VCARD_TIMEZONE_MAX || priv->timezone < MIMEDIR_VCARD_TIMEZONE_MIN)
			priv->timezone = G_MAXINT;
		break;
	case PROP_TIMEZONE_STRING:
		mimedir_utils_set_property_string (&priv->timezone_string, value);
		break;
	case PROP_LATITUDE:
		priv->latitude = g_value_get_double (value);
		break;
	case PROP_LONGITUDE:
		priv->longitude = g_value_get_double (value);
		break;

	case PROP_JOBTITLE:
		mimedir_utils_set_property_string (&priv->jobtitle, value);
		break;
	case PROP_JOBROLE:
		mimedir_utils_set_property_string (&priv->jobrole, value);
		break;
	case PROP_LOGO:
		g_string_free (priv->logo, TRUE);
		g_free (priv->logo_uri);
		priv->logo = NULL;
		priv->logo_uri = NULL;
		str = g_value_get_pointer (value);
		if (str)
			priv->logo = g_string_new_len (str->str, str->len);
		break;
	case PROP_LOGO_URI:
		g_string_free (priv->logo, TRUE);
		g_free (priv->logo_uri);
		priv->logo = NULL;
		priv->logo_uri = NULL;
		mimedir_utils_set_property_string (&priv->logo_uri, value);
		break;
	case PROP_AGENT:
		if (priv->agent)
			g_object_unref (G_OBJECT (priv->agent));
		priv->agent = g_value_get_object (value);
		if (priv->agent)
			g_object_ref (G_OBJECT (priv->agent));
		break;
	case PROP_AGENT_URI:
		mimedir_utils_set_property_string (&priv->agent_uri, value);
		break;
	case PROP_AGENT_STRING:
		mimedir_utils_set_property_string (&priv->agent_string, value);
		break;
	case PROP_ORGANIZATION_LIST:
		mimedir_utils_free_string_slist (priv->organization);
		priv->organization = mimedir_vcard_copy_string_list ((GSList *) g_value_get_pointer (value));
		break;
	case PROP_ORGANIZATION:
		mimedir_utils_free_string_slist (priv->organization);
		priv->organization = g_slist_append (NULL, g_strdup (g_value_get_string (value)));
		break;

	case PROP_CATEGORY_LIST:
		mimedir_utils_free_string_slist (priv->categories);
		priv->categories = mimedir_vcard_copy_string_list ((GSList *) g_value_get_pointer (value));
		break;
	case PROP_CATEGORIES:
		mimedir_utils_free_string_slist (priv->categories);
		priv->categories = mimedir_vcard_strsplit (g_value_get_string (value), ",");
		break;
	case PROP_NOTE:
		mimedir_utils_set_property_string (&priv->note, value);
		break;
	case PROP_SORT_STRING:
		mimedir_utils_set_property_string (&priv->sort_string, value);
		break;
	case PROP_SOUND:
		g_string_free (priv->sound, TRUE);
		g_free (priv->sound_uri);
		priv->sound = NULL;
		priv->sound_uri = NULL;
		str = g_value_get_pointer (value);
		if (str)
			priv->sound = g_string_new_len (str->str, str->len);
		break;
	case PROP_SOUND_URI:
		g_string_free (priv->sound, TRUE);
		g_free (priv->sound_uri);
		priv->sound = NULL;
		priv->sound_uri = NULL;
		mimedir_utils_set_property_string (&priv->sound_uri, value);
		break;
	case PROP_UID:
		mimedir_utils_set_property_string (&priv->uid, value);
		break;
	case PROP_URL:
		mimedir_utils_set_property_string (&priv->url, value);
		break;

	case PROP_CLASS:
		mimedir_utils_set_property_string (&priv->klass, value);
		break;
	case PROP_KEY_LIST:

		// Free Old List

		for (node = priv->keys; node != NULL; node = g_list_next (node)) {
			MIMEDirVCardKey *key = (MIMEDirVCardKey *) node->data;
			g_free (key->key);
			g_free (key);
		}
		g_list_free (priv->keys);
		priv->keys = NULL;

		// Copy List

		node = (GList *) g_value_get_pointer (value);
		for (; node != NULL; node = g_list_next (node)) {
			MIMEDirVCardKey *old, *new;

			g_return_if_fail (node->data != NULL);

			old = (MIMEDirVCardKey *) node->data;
			new = g_new (MIMEDirVCardKey, 1);
			new->key = g_strdup (old->key);
			new->type = old->type;

			priv->keys = g_list_prepend (priv->keys, new);
		}
		priv->keys = g_list_reverse (priv->keys);

		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}

	mimedir_vcard_changed (vcard);
}


static void
mimedir_vcard_get_property (GObject	*object,
			    guint	 property_id,
			    GValue	*value,
			    GParamSpec	*pspec)
{
	MIMEDirVCard *vcard;
	MIMEDirVCardPriv *priv;
	const gchar *s;
	gchar *str;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (object));

	vcard = MIMEDIR_VCARD (object);
	priv = vcard->priv;

	switch (property_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->fn);
		break;
	case PROP_FAMILYNAME:
		g_value_set_string (value, priv->familyname);
		break;
	case PROP_GIVENNAME:
		g_value_set_string (value, priv->givenname);
		break;
	case PROP_MIDDLENAME:
		g_value_set_string (value, priv->middlename);
		break;
	case PROP_PREFIX:
		g_value_set_string (value, priv->prefix);
		break;
	case PROP_SUFFIX:
		g_value_set_string (value, priv->suffix);
		break;
	case PROP_NICKNAME_LIST:
		g_value_set_pointer (value, priv->nickname);
		break;
	case PROP_NICKNAME:
		str = mimedir_vcard_strjoin (", ", priv->nickname);
		g_value_set_string (value, str);
		g_free (str);
		break;
	case PROP_PHOTO:
		g_value_set_pointer (value, priv->photo);
		break;
	case PROP_PHOTO_URI:
		g_value_set_string (value, priv->photo_uri);
		break;
	case PROP_BIRTHDAY:
		if (priv->birthday)
			g_value_set_object (value, G_OBJECT (priv->birthday));
		break;

	case PROP_ADDRESS_LIST:
		g_value_set_pointer (value, priv->addresses);
		break;
	case PROP_ADDRESS: {
		MIMEDirVCardAddress *address;
		gchar *s = NULL;

		address = mimedir_vcard_get_preferred_address (vcard);
		if (address)
			s = mimedir_vcard_address_get_as_string (address);
		g_value_set_string (value, s);
		g_free (s);
		break;
	}

	case PROP_PHONE_LIST:
		g_value_set_pointer (value, priv->phone);
		break;
	case PROP_PHONE: {
		MIMEDirVCardPhone *phone;
		gchar *s = NULL;

		phone = mimedir_vcard_get_preferred_phone (vcard);
		if (phone)
			s = mimedir_vcard_phone_get_as_string (phone);
		g_value_set_string (value, s);
		g_free (s);
		break;
	}
	case PROP_EMAIL_LIST:
		g_value_set_pointer (value, priv->email);
		break;
	case PROP_EMAIL: {
		MIMEDirVCardEMail *email;
		gchar *s = NULL;

		email = mimedir_vcard_get_preferred_email (vcard);
		if (email)
			s = mimedir_vcard_email_get_as_string (email);
		g_value_set_string (value, s);
		g_free (s);
		break;
	}
	case PROP_MAILER:
		g_value_set_string (value, priv->mailer);
		break;

	case PROP_TIMEZONE:
		g_value_set_int (value, priv->timezone);
		break;
	case PROP_TIMEZONE_STRING:
		g_value_set_string (value, priv->timezone_string);
		break;
	case PROP_LATITUDE:
		g_value_set_double (value, priv->latitude);
		break;
	case PROP_LONGITUDE:
		g_value_set_double (value, priv->longitude);
		break;

	case PROP_JOBTITLE:
		g_value_set_string (value, priv->jobtitle);
		break;
	case PROP_JOBROLE:
		g_value_set_string (value, priv->jobrole);
		break;
	case PROP_LOGO:
		g_value_set_pointer (value, priv->logo);
		break;
	case PROP_LOGO_URI:
		g_value_set_pointer (value, priv->logo_uri);
		break;
	case PROP_AGENT:
		g_value_set_object (value, priv->agent);
		break;
	case PROP_AGENT_URI:
		g_value_set_string (value, priv->agent_uri);
		break;
	case PROP_AGENT_STRING:
		g_value_set_string (value, priv->agent_string);
		break;
	case PROP_ORGANIZATION_LIST:
		g_value_set_pointer (value, priv->organization);
		break;
	case PROP_ORGANIZATION:
		if (priv->organization)
			s = (const gchar *) priv->organization->data;
		else
			s = NULL;
		g_value_set_string (value, s);
		break;

	case PROP_CATEGORY_LIST:
		g_value_set_pointer (value, priv->categories);
		break;
	case PROP_CATEGORIES:
		str = mimedir_vcard_strjoin (", ", priv->categories);
		g_value_set_string (value, str);
		g_free (str);
		break;
	case PROP_NOTE:
		g_value_set_string (value, priv->note);
		break;
	case PROP_PRODID:
		g_value_set_string (value, priv->prodid);
		break;
	case PROP_REVISION:
		g_value_set_object (value, priv->revision);
		break;
	case PROP_SORT_STRING:
		g_value_set_pointer (value, priv->sort_string);
		break;
	case PROP_SOUND:
		g_value_set_pointer (value, priv->sound);
		break;
	case PROP_SOUND_URI:
		g_value_set_pointer (value, priv->sound_uri);
		break;
	case PROP_UID:
		g_value_set_string (value, priv->uid);
		break;
	case PROP_URL:
		g_value_set_string (value, priv->url);
		break;

	case PROP_CLASS:
		g_value_set_string (value, priv->klass);
		break;
	case PROP_KEY_LIST:
		g_value_set_pointer (value, priv->keys);
		break;
	case PROP_PUBKEY:
		if (priv->keys) {
			MIMEDirVCardKey *key = (MIMEDirVCardKey *) priv->keys->data;
			g_value_set_string (value, key->key);
		}
		break;
	case PROP_KEYTYPE:
		if (priv->keys) {
			MIMEDirVCardKey *key = (MIMEDirVCardKey *) priv->keys->data;
			g_value_set_int (value, key->type);
		}
		break;

 	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Utility Functions
 */

static GSList *
mimedir_vcard_strsplit (const gchar *string, const gchar *separator)
{
	gchar **array, **a;
	GSList *list = NULL;

	array = g_strsplit (string, separator, 0);

	g_assert (array != NULL);

	for (a = array; *a != NULL; a++) {
		gchar *s;

		s = g_strdup ((const gchar *) *a);

		g_strstrip (s);

		list = g_slist_append (list, s);
	}

	g_strfreev (array);

	return list;
}


static gchar *
mimedir_vcard_strjoin (const gchar *separator, GSList *list)
{
	GString *s;

	s = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		s = g_string_append (s, (const gchar *) list->data);
		if (list->next)
			g_string_append (s, separator);
	}

	return g_string_free (s, FALSE);
}

/*
 * List Management
 */

static void
mimedir_vcard_internal_append_address (MIMEDirVCard *vcard, MIMEDirVCardAddress *address)
{
	MIMEDirVCardPriv *priv;
	gulong cid;

	priv = vcard->priv;

	g_object_ref (G_OBJECT (address));

	cid = g_signal_connect_swapped (G_OBJECT (address), "changed", G_CALLBACK (mimedir_vcard_changed), vcard);

	priv->addresses       = g_slist_append (priv->addresses, address);
	priv->address_signals = g_slist_append (priv->address_signals, g_memdup (&cid, sizeof (gulong)));
}


static void
mimedir_vcard_internal_remove_address (MIMEDirVCard *vcard, MIMEDirVCardAddress *address)
{
	MIMEDirVCardPriv *priv;
	GSList *list;
	gint idx;
	gulong *lp;

	priv = vcard->priv;

	idx = g_slist_index (priv->addresses, address);
	list = g_slist_nth (priv->address_signals, idx);
	lp = list->data;

	priv->addresses = g_slist_remove (priv->addresses, address);
	priv->address_signals = g_slist_delete_link (priv->address_signals, list);

	g_signal_handler_disconnect (address, *lp);
	g_free (lp);
	g_object_unref (G_OBJECT (address));
}


static void
mimedir_vcard_append_address_list (MIMEDirVCard *vcard, GSList *list)
{
	for (; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardAddress *address;

		g_return_if_fail (list->data != NULL && MIMEDIR_IS_VCARD_ADDRESS (list->data));

		address = MIMEDIR_VCARD_ADDRESS (list->data);
		mimedir_vcard_internal_append_address (vcard, address);
	}
}


static void
mimedir_vcard_free_address_list (MIMEDirVCard *vcard)
{
	while (vcard->priv->addresses) {
		gpointer p;

		p = vcard->priv->addresses->data;

		g_assert (p != NULL && MIMEDIR_IS_VCARD_ADDRESS (p));

		mimedir_vcard_internal_remove_address (vcard, MIMEDIR_VCARD_ADDRESS (p));
	}
}


static void
mimedir_vcard_internal_append_phone (MIMEDirVCard *vcard, MIMEDirVCardPhone *phone)
{
	MIMEDirVCardPriv *priv;
	gulong cid;

	priv = vcard->priv;

	g_object_ref (G_OBJECT (phone));

	cid = g_signal_connect_swapped (G_OBJECT (phone), "changed", G_CALLBACK (mimedir_vcard_changed), vcard);

	priv->phone         = g_slist_append (priv->phone, phone);
	priv->phone_signals = g_slist_append (priv->phone_signals, g_memdup (&cid, sizeof (gulong)));
}


static void
mimedir_vcard_internal_remove_phone (MIMEDirVCard *vcard, MIMEDirVCardPhone *phone)
{
	MIMEDirVCardPriv *priv;
	GSList *list;
	gint idx;
	gulong *lp;

	priv = vcard->priv;

	idx = g_slist_index (priv->phone, phone);
	list = g_slist_nth (priv->phone_signals, idx);
	lp = list->data;

	priv->phone = g_slist_remove (priv->phone, phone);
	priv->phone_signals = g_slist_delete_link (priv->phone_signals, list);

	g_signal_handler_disconnect (phone, *lp);
	g_free (lp);
	g_object_unref (G_OBJECT (phone));
}


static void
mimedir_vcard_append_phone_list (MIMEDirVCard *vcard, GSList *list)
{
	for (; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardPhone *phone;

		g_return_if_fail (list->data != NULL && MIMEDIR_IS_VCARD_PHONE (list->data));

		phone = MIMEDIR_VCARD_PHONE (list->data);
		mimedir_vcard_internal_append_phone (vcard, phone);
	}
}


static void
mimedir_vcard_free_phone_list (MIMEDirVCard *vcard)
{
	while (vcard->priv->phone) {
		gpointer p;

		p = vcard->priv->phone->data;

		g_assert (p != NULL && MIMEDIR_IS_VCARD_PHONE (p));

		mimedir_vcard_internal_remove_phone (vcard, MIMEDIR_VCARD_PHONE (p));
	}
}


static void
mimedir_vcard_internal_append_email (MIMEDirVCard *vcard, MIMEDirVCardEMail *email)
{
	MIMEDirVCardPriv *priv;
	gulong cid;

	priv = vcard->priv;

	g_object_ref (G_OBJECT (email));

	cid = g_signal_connect_swapped (G_OBJECT (email), "changed", G_CALLBACK (mimedir_vcard_changed), vcard);

	priv->email         = g_slist_append (priv->email, email);
	priv->email_signals = g_slist_append (priv->email_signals, g_memdup (&cid, sizeof (gulong)));
}


static void
mimedir_vcard_internal_remove_email (MIMEDirVCard *vcard, MIMEDirVCardEMail *email)
{
	MIMEDirVCardPriv *priv;
	GSList *list;
	gint idx;
	gulong *lp;

	priv = vcard->priv;

	idx = g_slist_index (priv->email, email);
	list = g_slist_nth (priv->email_signals, idx);
	lp = list->data;

	priv->email = g_slist_remove (priv->email, email);
	priv->email_signals = g_slist_delete_link (priv->email_signals, list);

	g_signal_handler_disconnect (email, *lp);
	g_free (lp);
	g_object_unref (G_OBJECT (email));
}


static void
mimedir_vcard_append_email_list (MIMEDirVCard *vcard, GSList *list)
{
	for (; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardEMail *email;

		g_return_if_fail (list->data != NULL && MIMEDIR_IS_VCARD_EMAIL (list->data));

		email = MIMEDIR_VCARD_EMAIL (list->data);
		mimedir_vcard_internal_append_email (vcard, email);
	}
}


static void
mimedir_vcard_free_email_list (MIMEDirVCard *vcard)
{
	while (vcard->priv->email) {
		gpointer p;

		p = vcard->priv->email->data;

		g_assert (p != NULL && MIMEDIR_IS_VCARD_EMAIL (p));

		mimedir_vcard_internal_remove_email (vcard, MIMEDIR_VCARD_EMAIL (p));
	}
}

/*
 * Private Methods
 */

/* Free VCard's allocated memory */
static void
mimedir_vcard_clear (MIMEDirVCard *vcard)
{
	MIMEDirVCardPriv *priv;
	GList *node;

	priv = vcard->priv;

	if (!priv)
		return;

	g_free (priv->name);
	g_free (priv->source);

	g_free (priv->fn);
	g_free (priv->familyname);
	g_free (priv->givenname);
	g_free (priv->middlename);
	g_free (priv->prefix);
	g_free (priv->suffix);
	mimedir_utils_free_string_slist (priv->nickname);
	if (priv->photo)
		g_string_free (priv->photo, TRUE);
	g_free (priv->photo_uri);
	if (priv->birthday)
		g_object_unref (G_OBJECT (priv->birthday));

	mimedir_vcard_free_address_list (vcard);

	mimedir_vcard_free_phone_list (vcard);
	mimedir_vcard_free_email_list (vcard);
	g_free (priv->mailer);

	g_free (priv->jobtitle);
	g_free (priv->jobrole);
	if (priv->logo)
		g_string_free (priv->logo, TRUE);
	g_free (priv->logo_uri);
	if (priv->agent)
		g_object_unref (G_OBJECT (priv->agent));
	g_free (priv->agent_string);
	g_free (priv->agent_uri);
	mimedir_utils_free_string_slist (priv->organization);

	mimedir_utils_free_string_slist (priv->categories);
	g_free (priv->note);
	g_free (priv->prodid);
	if (priv->revision)
		g_object_unref (G_OBJECT (priv->revision));
	g_free (priv->sort_string);
	if (priv->sound)
		g_string_free (priv->sound, TRUE);
	g_free (priv->sound_uri);
	g_free (priv->uid);
	g_free (priv->url);

	g_free (priv->klass);
	for (node = priv->keys; node != NULL; node = g_list_next (node)) {
		MIMEDirVCardKey *key = (MIMEDirVCardKey *) node->data;
		g_free (key->key);
		g_free (key);
	}
	g_list_free (priv->keys);

	g_free (priv);
	vcard->priv = NULL;
}


/* Reallocate memory for the VCard */
static void
mimedir_vcard_reset (MIMEDirVCard *vcard)
{
	mimedir_vcard_clear (vcard);
	mimedir_vcard_init (vcard);
}


/* Emit the changed signal */
static void
mimedir_vcard_changed (MIMEDirVCard *vcard)
{
	vcard->priv->changed = TRUE;
	g_signal_emit (G_OBJECT (vcard), mimedir_vcard_signals[SIGNAL_CHANGED], 0);
}


static void
mimedir_vcard_update_revision (MIMEDirVCard *vcard)
{
	MIMEDirVCardPriv *priv;
	time_t t;
	struct tm tm;

	priv = vcard->priv;

	t = time (NULL);

	gmtime_r (&t, &tm);

	if (!priv->revision)
		priv->revision = mimedir_datetime_new_from_struct_tm (&tm);
}

/*
 * Parsing
 */

static gint
mimedir_vcard_parse_utc_offset (MIMEDirAttribute *attr, GError **error)
{
	const gchar *attrname, *s;
	gboolean positive = TRUE;
	gint hour, minute;

	attrname = mimedir_attribute_get_name (attr);
	s = mimedir_attribute_get_value (attr);

	if (*s == '-') {
		positive = FALSE;
		s++;
	} else if (*s == '+') {
		positive = TRUE;
		s++;
	}

	if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "utc-offset", attrname);
		return G_MAXINT;
	}
	hour = (s[0] - '0') * 10;
	hour += s[1] - '0';
	s += 2;

	if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "utf-offset", attrname);
		return G_MAXINT;
	}
	minute = (s[0] - '0') * 10;
	minute += s[1] - '0';
	s += 2;

	if (hour > 23 || minute > 59) {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "utf-offset", attrname);
		return G_MAXINT;
	}

	if (*s != '\0') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "utc-offset", attrname);
		return G_MAXINT;
	}

	if (positive)
		return hour * 60 + minute;
	else
		return -(hour * 60 + minute);
}

/*
 * Static Methods
 */

/* This function is documented elsewhere. */
GList *
mimedir_vcard_read_file (const gchar *filename, GError **error)
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
			(*error)->message = g_strdup_printf (_("Error reading card file %s: %s!"), filename, msg);
			g_free (msg);
		}
		return NULL;
	}

	list = mimedir_vcard_read_channel (channel, &err);

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
				(*error)->message = g_strdup_printf (_("Error reading card file %s: %s!"), filename, msg);
				g_free (msg);
			}
			return NULL;
		}

		G_CONST_RETURN char *encoding;
		g_get_charset(&encoding);
		if (g_io_channel_set_encoding (channel, encoding, error) != G_IO_STATUS_NORMAL) {
			g_io_channel_unref (channel);
			return NULL;
		}

		list = mimedir_vcard_read_channel (channel, &err);

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
				(*error)->message = g_strdup_printf (_("Error reading card file %s: %s!"), filename, msg);
				g_free (msg);
			}
			return NULL;
		}

		if (g_io_channel_set_encoding (channel, "ISO-8859-15", error) != G_IO_STATUS_NORMAL) {
			g_io_channel_unref (channel);
			return NULL;
		}

		list = mimedir_vcard_read_channel (channel, &err);

		g_io_channel_unref (channel);
	}

	/* Error handling */

	if (err) {
		gchar *msg;

		g_propagate_error (error, err);

		msg = (*error)->message;
		(*error)->message = g_strdup_printf (_("Error reading card file %s: %s!"), filename, msg);
		g_free (msg);
	}

	return list;
}

/**
 * mimedir_vcard_read_channel:
 * @channel: I/O channel to read from
 * @error: error storage location or %NULL
 *
 * Reads a list of vCards from the supplied I/O channel and returns it. @error
 * will be set if an error occurs. You should free the returned list with
 * mimedir_vcard_free_list().
 *
 * Return value: list of read vCards
 **/
GList *
mimedir_vcard_read_channel (GIOChannel *channel, GError **error)
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
		MIMEDirVCard *vcard;
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
			mimedir_vcard_free_list (list);
			list = NULL;
			break;
		}
		pos += newpos;

		vcard = mimedir_vcard_new_from_profile (profile, error);

		g_object_unref (G_OBJECT (profile));

		if (!vcard) {
			mimedir_vcard_free_list (list);
			list = NULL;
			break;
		}

		list = g_list_append (list, vcard);
	}

	g_free (string);

	return list;
}

/**
 * mimedir_vcard_free_list:
 * @list: list of vCards
 *
 * Frees a list of vCards as returned by mimedir_vcard_read_list_from_file()
 * or mimedir_vcard_read_list_from_channel().
 **/
void
mimedir_vcard_free_list (GList *list)
{
	GList *current;

	for (current = list; current != NULL; current = g_list_next (current))
		g_object_unref (G_OBJECT (current->data));

	g_list_free (list);
}

/**
 * mimedir_vcard_write_list:
 * @filename: file to save to
 * @list: list of vCard objects
 * @error: error storage location or %NULL
 *
 * Saves a list of vCard to a file.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_write_list (const gchar *filename, GList *list, GError **error)
{
	gboolean success;
	GIOChannel *channel;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	channel = g_io_channel_new_file (filename, "w", error);
	if (!channel) {
		if (error && *error) {
			gchar *msg = (*error)->message;
			(*error)->message = g_strdup_printf (_("Error writing card file %s: %s!"), filename, msg);
			g_free (msg);
		}
		return FALSE;
	}

	success = mimedir_vcard_write_list_to_channel (channel, list, error);

	g_io_channel_unref (channel);

	return success;
}

/**
 * mimedir_vcard_write_list_to_channel:
 * @channel: I/O channel to save to
 * @list: list of vCard objects
 * @error: error storage location or %NULL
 *
 * Saves a list of vCard to an I/O channel.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_write_list_to_channel (GIOChannel *channel, GList *list, GError **error)
{
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	for (; list != NULL; list = g_list_next (list)) {
		MIMEDirVCard *vcard;

		if (list->data == NULL || !MIMEDIR_IS_VCARD (list->data)) {
			g_critical ("list must contain only valid vCard objects");
			return FALSE;
		}

		vcard = MIMEDIR_VCARD (list->data);
		if (!mimedir_vcard_write_to_channel (vcard, channel, error))
			return FALSE;
	}

	return TRUE;
}

/*
 * Public Methods
 */

/**
 * mimedir_vcard_new:
 *
 * Creates a new (empty) vCard object.
 *
 * Return value: a new vCard object
 **/
MIMEDirVCard *
mimedir_vcard_new (void)
{
	MIMEDirVCard *vcard;

	vcard = g_object_new (MIMEDIR_TYPE_VCARD, NULL);

	return vcard;
}

/**
 * mimedir_vcard_new_from_profile:
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Create a new vCard object and fills it with data retrieved from the
 * supplied profile object. If an error occurs during the read, @error
 * will be set and %NULL will be returned.
 *
 * Return value: the new vCard object or %NULL
 **/
MIMEDirVCard *
mimedir_vcard_new_from_profile (MIMEDirProfile *profile, GError **error)
{
	MIMEDirVCard *vcard;

	g_return_val_if_fail (profile != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	vcard = g_object_new (MIMEDIR_TYPE_VCARD, NULL);

	if (!mimedir_vcard_read_from_profile (vcard, profile, error)) {
		g_object_unref (G_OBJECT (vcard));
		vcard = NULL;
	}

	return vcard;
}

/**
 * mimedir_vcard_read_from_profile:
 * @vcard: a vCard
 * @profile: a profile object
 * @error: error storage location or %NULL
 *
 * Clears the supplied vCard object and re-initializes it with data read
 * from the supplied profile. If an error occurs during the read, @error
 * will be set and %FALSE will be returned. Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_read_from_profile (MIMEDirVCard *vcard, MIMEDirProfile *profile, GError **error)
{
	MIMEDirVCardPriv *priv;
	GSList *attrs;
	gchar *name;

	g_return_val_if_fail (vcard != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), FALSE);
	g_return_val_if_fail (profile != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_PROFILE (profile), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	mimedir_vcard_reset (vcard);
	priv = vcard->priv;

	g_object_get (G_OBJECT (profile), "name", &name, NULL);
	if (name && g_ascii_strcasecmp (name, "VCARD") != 0) {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, name, "VCARD");
		g_free (name);
		return FALSE;
	}
	g_free (name);

	attrs = mimedir_profile_get_attribute_list (profile);

	for (; attrs != NULL; attrs = g_slist_next (attrs)) {
		GError *err = NULL;
		MIMEDirAttribute *attr;
		const gchar *name;

		attr = MIMEDIR_ATTRIBUTE (attrs->data);

		name = mimedir_attribute_get_name (attr);

		/*
		 * Generic Types (RFC 2425, Section 6; RFC 2426, Section 2.1)
		 */

		/* Profile Begin (6.4, 2.1.1) */
		if (!g_ascii_strcasecmp (name, "BEGIN")) {
			/* ignore */
		}

		/* Profile End (6.5, 2.1.1) */
		else if (!g_ascii_strcasecmp (name, "END")) {
			/* ignore */
		}

		/* Name (6.2, 2.1.2) */
		else if (!g_ascii_strcasecmp (name, "NAME")) {
			if (priv->name != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->name = mimedir_attribute_get_value_text (attr, error);
			if (!priv->name)
				return FALSE;
		}

		/* Profile Name (6.3, 2.1.3) */
		else if (!g_ascii_strcasecmp (name, "PROFILE")) {
			const gchar *profname;

			profname = mimedir_attribute_get_value (attr);
			if (g_ascii_strcasecmp (profname, "VCARD") != 0) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE, MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR, profname, "VCARD");
				return FALSE;
			}
		}

		/* Source (6.1, 2.1.4) */
		else if (!g_ascii_strcasecmp (name, "SOURCE")) {
			if (priv->source != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->source = mimedir_attribute_get_value_uri (attr, error);
			if (!priv->source)
				return FALSE;
		}

		/*
		 * Identification Types (RFC 2426, Section 3.1)
		 */

		/* Name (3.1.1) */
		else if (!g_ascii_strcasecmp (name, "FN")) {
			if (priv->fn != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->fn = mimedir_attribute_get_value_text (attr, error);
			if (!priv->fn)
				return FALSE;
		}

		/* Structured Name (3.1.2) */
		else if (!g_ascii_strcasecmp (name, "N")) {
			GSList *list, *current;

			if (priv->familyname != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			list = mimedir_attribute_get_value_structured_text (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
			current = list;
			priv->familyname = mimedir_vcard_strjoin (" ", current->data);
			/* Note: this code does not complain if fewer than 5 elements are present.
			   This is deliberate: Palm generates N: lines with one element */
			if (current->next) {
				current = g_slist_next (current);
				priv->givenname = mimedir_vcard_strjoin (" ", current->data);
			}
			if (current->next) {
				current = g_slist_next (current);
				priv->middlename = mimedir_vcard_strjoin (" ", current->data);
			}
			if (current->next) {
				current = g_slist_next (current);
				priv->prefix = mimedir_vcard_strjoin (" ", current->data);
			}
			if (current->next) {
				current = g_slist_next (current);
				priv->suffix = mimedir_vcard_strjoin (" ", current->data);
			}
			if (current->next) {
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG_STR, name);
				return FALSE;
			}
			mimedir_attribute_free_structured_text_list (list);
		}

		/* Nick Name (3.1.3) */
		else if (!g_ascii_strcasecmp (name, "NICKNAME")) {
			GSList *attr_list, *copy_list;

			attr_list = mimedir_attribute_get_value_text_list (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
			
			copy_list = mimedir_vcard_copy_string_list (attr_list);
			mimedir_attribute_free_list (attr_list);

			priv->nickname = g_slist_concat (priv->nickname, copy_list);
		}

		/* Photo (3.1.4) */
		else if (!g_ascii_strcasecmp (name, "PHOTO")) {
			if (priv->photo || priv->photo_uri) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			if (mimedir_attribute_get_attribute_type (attr) == MIMEDIR_ATTRIBUTE_TYPE_URI) {
				priv->photo_uri = mimedir_attribute_get_value_uri (attr, error);
				if (!priv->photo_uri)
					return FALSE;
			} else {
				const gchar *data;
				gssize len;
		
				data = mimedir_attribute_get_value_data (attr, &len);
				priv->photo = g_string_new_len (data, len);
			}
		}

		/* Birthday (3.1.5) */
		else if (!g_ascii_strcasecmp (name, "BDAY")) {
			if (priv->birthday) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->birthday = mimedir_attribute_get_value_datetime (attr, &err);
			if (!priv->birthday) {
				g_propagate_error (error, err);
				return FALSE;
			}
			if (!(priv->birthday->flags & MIMEDIR_DATETIME_DATE)) {
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
				return FALSE;
			}
		}

		/*
		 * Delivery Addressing Types (RFC 2426, Section 3.2)
		 */

		/* Address (3.2.1), Address Label (3.2.2) */
		else if (!g_ascii_strcasecmp (name, "ADR") ||
			 !g_ascii_strcasecmp (name, "LABEL")) {
			MIMEDirVCardAddress *address;

			address = mimedir_vcard_address_new_from_attribute (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			mimedir_vcard_internal_append_address (vcard, address);

			g_object_unref (G_OBJECT (address));
		}

		/*
		 * Telecommunications Addressing Types (RFC 2426, Section 3.3)
		 */

		/* Telephone Number (3.3.1) */
		else if (!g_ascii_strcasecmp (name, "TEL")) {
			MIMEDirVCardPhone *phone;

			phone = mimedir_vcard_phone_new_from_attribute (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			mimedir_vcard_internal_append_phone (vcard, phone);

			g_object_unref (G_OBJECT (phone));
		}

		/* E-mail Address (3.3.2) */
		else if (!g_ascii_strcasecmp (name, "EMAIL")) {
			MIMEDirVCardEMail *email;

			email = mimedir_vcard_email_new_from_attribute (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			mimedir_vcard_internal_append_email (vcard, email);

			g_object_unref (G_OBJECT (email));
		}

		/* Mailer (3.3.3) */
		else if (!g_ascii_strcasecmp (name, "MAILER")) {
			const gchar *s;

			if (priv->mailer != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			s = mimedir_attribute_get_value (attr);
			priv->mailer = g_strdup (s);
		}

		/*
		 * Geographical Types (RFC 2426, Section 3.4)
		 */

		/* Time Zone (3.4.1) */
		else if (!g_ascii_strcasecmp (name, "TZ")) {
			const gchar *type;

			if (priv->timezone != G_MAXINT || priv->timezone_string != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			type = mimedir_attribute_get_parameter_value (attr, "type");

			if (!g_ascii_strcasecmp (type, "text")) {
				priv->timezone_string = mimedir_attribute_get_value_text (attr, error);
				if (!priv->timezone_string)
					return FALSE;
			} else {
				gint offset;

				offset = mimedir_vcard_parse_utc_offset (attr, error);

				if (offset == G_MAXINT)
					return FALSE;

				priv->timezone = offset;
			}
		}

		/* Geographical Position (3.4.2) */
		else if (!g_ascii_strcasecmp (name, "GEO")) {
			GSList *list;
			gdouble latitude, longitude;

			list = mimedir_attribute_get_value_float_list (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			if (g_slist_length (list) < 2) {
				mimedir_attribute_free_float_list (list);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT_STR, name);
				return FALSE;
			} else if (g_slist_length (list) > 2) {
				mimedir_attribute_free_float_list (list);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG, MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG_STR, name);
				return FALSE;
			}

			latitude  = *((gdouble *) list->data);
			longitude = *((gdouble *) list->next->data);

			mimedir_attribute_free_float_list (list);

			if (latitude  <  -90.0 || latitude  >  +90.0 ||
			    longitude < -180.0 || longitude > +180.0) {
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, name);
				return FALSE;
			}

			priv->latitude  = latitude;
			priv->longitude = longitude;
		}

		/*
		 * Organizational Types (RFC 2426, Section 3.5)
		 */

		/* Job Title (3.5.1) */
		else if (!g_ascii_strcasecmp (name, "TITLE")) {
			if (priv->jobtitle != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->jobtitle = mimedir_attribute_get_value_text (attr, error);
			if (!priv->jobtitle)
				return FALSE;
		}

		/* Job Role (3.5.2) */
		else if (!g_ascii_strcasecmp (name, "ROLE")) {
			if (priv->jobrole != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->jobrole = mimedir_attribute_get_value_text (attr, error);
			if (!priv->jobrole)
				return FALSE;
		}

		/* Organization Logo (3.5.3) */
		else if (!g_ascii_strcasecmp (name, "LOGO")) {
			if (priv->logo || priv->logo_uri) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			if (mimedir_attribute_get_attribute_type (attr) == MIMEDIR_ATTRIBUTE_TYPE_URI) {
				priv->logo_uri = mimedir_attribute_get_value_uri (attr, error);
				if (!priv->logo_uri)
					return FALSE;
			} else {
				gchar *s;
				gssize len;

				s = mimedir_attribute_get_value_data (attr, &len);
				priv->logo = g_string_new_len (s, len);
			}
		}

		/* Agent (3.5.4) */
		else if (!g_ascii_strcasecmp (name, "AGENT")) {
			MIMEDirAttributeType type;

			if (priv->agent || priv->agent_uri || priv->agent_string) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			type = mimedir_attribute_get_attribute_type (attr);
			if (type == MIMEDIR_ATTRIBUTE_TYPE_URI)
				priv->agent_uri = mimedir_attribute_get_value_uri (attr, &err);
			else if (type == MIMEDIR_ATTRIBUTE_TYPE_TEXT)
				priv->agent_string = mimedir_attribute_get_value_text (attr, &err);
			else {
				gchar *s;

				s = mimedir_attribute_get_value_text (attr, &err);
				if (!err) {
					MIMEDirProfile *profile;

					profile = mimedir_profile_new (NULL);
					mimedir_profile_parse (profile, s, &err);
					if (!err)
						priv->agent = mimedir_vcard_new_from_profile (profile, &err);
				}
			}

			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
		}

		/* Organization (3.5.5) */
		else if (!g_ascii_strcasecmp (name, "ORG")) {
			GSList *list, *current;

			if (priv->organization != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			/* For compatibility reasons, we treat commas in the
			 * ORG type as literal characters.
			 */

			list = mimedir_attribute_get_value_structured_text (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			for (current = list; current != NULL; current = g_slist_next (current)) {
				GSList *ilist;
				GString *string;

				string = g_string_new ("");

				for (ilist = (GSList *) current->data; ilist != NULL; ilist = g_slist_next (ilist)) {
					g_string_append (string, (const gchar *) ilist->data);
					if (ilist->next)
						g_string_append_c (string, ',');
				}

				priv->organization = g_slist_append (priv->organization, g_string_free (string, FALSE));
			}
		}

		/*
		 * Explanatory Types (RFC 2426, Section 3.6)
		 */

		/* Categories (3.6.1) */
		else if (!g_ascii_strcasecmp (name, "CATEGORIES")) {
			GSList *attr_list, *copy_list;

			attr_list = mimedir_attribute_get_value_text_list (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			copy_list = mimedir_vcard_copy_string_list (attr_list);
			mimedir_attribute_free_list (attr_list);

			priv->categories = g_slist_concat (priv->categories, copy_list);
		}

		/* Note (3.6.2) */
		else if (!g_ascii_strcasecmp (name, "NOTE")) {
			GSList *list, *l;
			GString *s;

			if (priv->note != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			list = mimedir_attribute_get_value_text_list (attr, &err);

			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}

			/* Treat not-escaped commas like escaped commas,
			 * for compatibility reasons.
			 */

			s = g_string_new (NULL);
			for (l = list; l != NULL; l = g_slist_next (l)) {
				g_string_append (s, (const gchar *) l->data);
				if (l->next)
					g_string_append_c (s, ',');
			}
			priv->note = g_string_free (s, FALSE);

			mimedir_attribute_free_list (list);
		}

		/* Product ID (3.6.3) */
		else if (!g_ascii_strcasecmp (name, "PRODID")) {
			if (priv->prodid) {
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->prodid = mimedir_attribute_get_value_text (attr, error);
			if (!priv->prodid) {
				g_propagate_error (error, err);
				return FALSE;
			}
		}

		/* Revision (3.6.4) */
		else if (!g_ascii_strcasecmp (name, "REV")) {
			if (priv->revision) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->revision = mimedir_attribute_get_value_datetime (attr, &err);
			if (!priv->revision) {
				g_propagate_error (error, err);
				return FALSE;
			}
			if (!(priv->revision->flags & MIMEDIR_DATETIME_DATE)) {
				g_object_unref (G_OBJECT (priv->revision));
				priv->revision = NULL;
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date-time", name);
				return FALSE;
			}
			priv->revision->flags &= ~MIMEDIR_DATETIME_TIME;
		}

		/* Sort string (3.6.5) */
		else if (!g_ascii_strcasecmp (name, "SORT-STRING")) {
			if (priv->sort_string != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->sort_string = mimedir_attribute_get_value_text (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
		}

		/* Sound (3.6.6) */
		else if (!g_ascii_strcasecmp (name, "SOUND")) {
			if (priv->sound || priv->sound_uri) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			if (mimedir_attribute_get_attribute_type (attr) == MIMEDIR_ATTRIBUTE_TYPE_URI) {
				priv->sound_uri = mimedir_attribute_get_value_uri (attr, &err);
				if (!priv->sound_uri)
					return FALSE;
			} else {
				const gchar *s;
				gssize len;

				s = mimedir_attribute_get_value_data (attr, &len);
				priv->sound = g_string_new_len (s, len);
			}
		}

		/* UID (3.6.7) */
		else if (!g_ascii_strcasecmp (name, "UID")) {
			if (priv->uid != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->uid = mimedir_attribute_get_value_text (attr, error);
			if (!priv->uid)
				return FALSE;
		}

		/* URL (3.6.8) */
		else if (!g_ascii_strcasecmp (name, "URL")) {
			if (priv->url != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->url = mimedir_attribute_get_value_uri (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
		}

		/* Version (3.6.9) */
		else if (!g_ascii_strcasecmp (name, "VERSION")) {
			/* no op */
		}

		/*
		 * Security Types (RFC 2426, Section 3.7)
		 */

		/* Access Classification (3.7.1) */
		else if (!g_ascii_strcasecmp (name, "CLASS")) {
			if (priv->klass != NULL) {
				g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE, MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR, name, mimedir_attribute_get_value(attr));
				return FALSE;
			}

			priv->klass = mimedir_attribute_get_value_uri (attr, &err);
			if (err) {
				g_propagate_error (error, err);
				return FALSE;
			}
		}

		/* Key (3.7.2) */
		else if (!g_ascii_strcasecmp (name, "KEY")) {
			MIMEDirVCardKey *key;
			const gchar *type, *s;
			gssize len;

			s = mimedir_attribute_get_value_data (attr, &len);

			key = g_new (MIMEDirVCardKey, 1);
			key->key = g_strdup (s);
			key->type = MIMEDIR_VCARD_KEY_UNKNOWN;

			type = mimedir_attribute_get_parameter_value (attr, "type");
			if (type && type[0] != '\0') {
				if (g_ascii_strcasecmp (type, "X509") == 0)
					key->type = MIMEDIR_VCARD_KEY_X509;
				else if (g_ascii_strcasecmp (type, "PGP") == 0)
					key->type = MIMEDIR_VCARD_KEY_PGP;
			}

			priv->keys = g_list_append (priv->keys, key);
		}

		else if (!g_ascii_strncasecmp (name, "X-", 2))
			g_printerr (_("The profile contains the unsupported custom attribute %s.\n"), name);

		else
			g_printerr (_("The profile contains the unknown attribute %s.\n"), name);
	}

	if (priv->familyname == NULL) {
	  /* Palm has a habit of leaving out the N: attribute, even though RFC 2426 says it is mandatory.
	     So, let's try to work it out from the FN: or ORG: if present */
	  if (priv->fn) priv->familyname = g_strdup(priv->fn);
	  else if (priv->organization) priv->familyname = mimedir_vcard_strjoin (" ", priv->organization);
	  else {
		g_set_error (error, MIMEDIR_PROFILE_ERROR, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING, MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING_STR, "N");
		return FALSE;
	  }
	}
	if (priv->fn == NULL) {
		/* According to RFC 2426, every VCard must have a FN attribute.
		 * But because we are nice, we construct the full name if it
		 * wasn't given, using European heuristics:
		 * PREFIXES GIVENNAME ADDITIONAL FAMILYNAME SUFFIXES
		 */

		GString *s;

		s = g_string_new (NULL);

		if (priv->prefix) {
			g_string_append (s, priv->prefix);
			g_string_append_c (s, ' ');
		}
		if (priv->givenname) {
			g_string_append (s, priv->givenname);
			g_string_append_c (s, ' ');
		}
		if (priv->middlename) {
			g_string_append (s, priv->middlename);
			g_string_append_c (s, ' ');
		}
		g_string_append (s, priv->familyname);
		if (priv->suffix) {
			g_string_append (s, priv->suffix);
			g_string_append_c (s, ' ');
		}

		priv->fn = g_string_free (s, FALSE);
	}

	g_signal_emit (G_OBJECT (vcard), mimedir_vcard_signals[SIGNAL_CHANGED], 0);

	return TRUE;
}

/**
 * mimedir_vcard_write_to_profile:
 * @vcard: a vCard
 *
 * Saves the vCard object to a newly allocated profile object.
 *
 * Return value: a new profile
 **/
MIMEDirProfile *
mimedir_vcard_write_to_profile (MIMEDirVCard *vcard)
{
	MIMEDirVCardPriv *priv;
	MIMEDirProfile *profile;
	MIMEDirAttribute *attr;
	GSList *list, *list2;
	GList *node;

	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	priv = vcard->priv;

	if (priv->changed || priv->revision->year == 0)
		mimedir_vcard_update_revision (vcard);

	profile = mimedir_profile_new ("VCARD");

	/*
	 * Generic Types (RFC 2425, Section 6; RFC 2426, Section 2.1)
	 */

	/* Name (6.2, 2.1.2) */

	if (priv->name) {
		attr = mimedir_attribute_new_with_name ("NAME");
		mimedir_attribute_set_value_text (attr, priv->name);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Profile Name (6.3, 2.1.3) */

	attr = mimedir_attribute_new_with_name ("PROFILE");
	mimedir_attribute_set_value (attr, "VCARD");
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/* Source (6.1, 2.1.4) */

	if (priv->source) {
		attr = mimedir_attribute_new_with_name ("SOURCE");
		mimedir_attribute_set_value_uri (attr, priv->source);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Identification Types (RFC 2426, Section 3.1)
	 */

	/* Name (3.1.1) */

	g_assert (priv->fn != NULL);

	attr = mimedir_attribute_new_with_name ("FN");
	mimedir_attribute_set_value_text (attr, priv->fn);
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/* Structured Name (3.1.2) */

	list = NULL;
	list2 = NULL;
	list2 = g_slist_append (list2, priv->familyname ? priv->familyname : "");
	list = g_slist_append (list, list2);
	list2 = NULL;
	list2 = g_slist_append (list2, priv->givenname  ? priv->givenname  : "");
	list = g_slist_append (list, list2);
	list2 = NULL;
	list2 = g_slist_append (list2, priv->middlename ? priv->middlename : "");
	list = g_slist_append (list, list2);
	list2 = NULL;
	list2 = g_slist_append (list2, priv->prefix     ? priv->prefix     : "");
	list = g_slist_append (list, list2);
	list2 = NULL;
	list2 = g_slist_append (list2, priv->suffix     ? priv->suffix     : "");
	list = g_slist_append (list, list2);

	attr = mimedir_attribute_new_with_name ("N");
	mimedir_attribute_set_value_structured_text (attr, list);
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	for (list2 = list; list2 != NULL; list2 = g_slist_next (list2))
		g_slist_free (list2->data);
	g_slist_free (list);

	/* Nick Name (3.1.3) */

	if (priv->nickname) {
		attr = mimedir_attribute_new_with_name ("NICKNAME");
		mimedir_attribute_set_value_text_list (attr, priv->nickname);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Photo (3.1.4) */

	if (priv->photo) {
		attr = mimedir_attribute_new_with_name ("PHOTO");
		mimedir_attribute_set_value_data (attr, priv->photo->str, priv->photo->len);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	} else if (priv->photo_uri) {
		attr = mimedir_attribute_new_with_name ("PHOTO");
		mimedir_attribute_set_value_uri (attr, priv->photo_uri);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "uri");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Birthday (3.1.5) */

	if (priv->birthday) {
		attr = mimedir_attribute_new_with_name ("BDAY");
		mimedir_attribute_set_value_datetime (attr, priv->birthday);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Delivery Addressing Types (RFC 2426, Section 3.2)
	 */

	/* Address (3.2.1), Address Label (3.2.2) */

	for (list = priv->addresses; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardAddress *address;

		g_assert (list->data != NULL && MIMEDIR_IS_VCARD_ADDRESS (list->data));
		address = MIMEDIR_VCARD_ADDRESS (list->data);

		attr = mimedir_vcard_address_save_to_attribute (address);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Telecommunications Addressing Types (RFC 2426, Section 3.3)
	 */

	/* Telephone Number (3.3.1) */

	for (list = priv->phone; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardPhone *phone;

		g_assert (list->data != NULL && MIMEDIR_IS_VCARD_PHONE (list->data));
		phone = MIMEDIR_VCARD_PHONE (list->data);

		attr = mimedir_vcard_phone_save_to_attribute (phone);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* E-mail Address (3.3.2) */

	for (list = priv->email; list != NULL; list = g_slist_next (list)) {
		MIMEDirVCardEMail *email;

		g_assert (list->data != NULL && MIMEDIR_IS_VCARD_EMAIL (list->data));
		email = MIMEDIR_VCARD_EMAIL (list->data);

		attr = mimedir_vcard_email_save_to_attribute (email);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Mailer (3.3.3) */

	if (priv->mailer && priv->mailer[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("MAILER");
		mimedir_attribute_set_value_text (attr, priv->mailer);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Geographical Types (RFC 2426, Section 3.4)
	 */

	/* Time Zone (3.4.1) */

	if (priv->timezone >= MIMEDIR_VCARD_TIMEZONE_MIN &&
	    priv->timezone <= MIMEDIR_VCARD_TIMEZONE_MAX) {
		gchar *s;
		gchar sign = '+';
		gint hour, minute;

		hour   = priv->timezone / 60;
		minute = priv->timezone % 60;

		if (priv->timezone < 0) {
			sign   = '-';
			hour   = -hour;
			minute = -minute;
		}

		s = g_strdup_printf ("%c%02d:%02d", sign, hour, minute);
		attr = mimedir_attribute_new_with_name ("TZ");
		mimedir_attribute_set_value (attr, s);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));

		g_free (s);
	} else if (priv->timezone_string && priv->timezone_string[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("TZ");
		mimedir_attribute_set_value_text (attr, priv->timezone_string);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "text");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Geographical Position (3.4.2) */

	if (priv->latitude  >= MIMEDIR_VCARD_LATITUDE_MIN  &&
	    priv->latitude  <= MIMEDIR_VCARD_LATITUDE_MAX  &&
	    priv->longitude >= MIMEDIR_VCARD_LONGITUDE_MIN &&
	    priv->longitude <= MIMEDIR_VCARD_LONGITUDE_MAX) {
		list = NULL;
		list = g_slist_append (list, &priv->latitude);
		list = g_slist_append (list, &priv->longitude);

		attr = mimedir_attribute_new_with_name ("GEO");
		mimedir_attribute_set_value_float_list (attr, list);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/*
	 * Organizational Types (RFC 2426, Section 3.5)
	 */

	/* Job Title (3.5.1) */

	if (priv->jobtitle && priv->jobtitle[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("TITLE");
		mimedir_attribute_set_value_text (attr, priv->jobtitle);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Job Role (3.5.2) */

	if (priv->jobrole && priv->jobrole[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("ROLE");
		mimedir_attribute_set_value_text (attr, priv->jobrole);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Organization Logo (3.5.3) */

	if (priv->logo) {
		attr = mimedir_attribute_new_with_name ("LOGO");
		mimedir_attribute_set_value_data (attr, priv->logo->str, priv->logo->len);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	} else if (priv->logo_uri) {
		attr = mimedir_attribute_new_with_name ("LOGO");
		mimedir_attribute_set_value_uri (attr, priv->logo_uri);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "uri");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Agent (3.5.4) */

	if (priv->agent) {
		gchar *s;

		attr = mimedir_attribute_new_with_name ("AGENT");
		s = mimedir_vcard_get_as_string (priv->agent);
		mimedir_attribute_set_value_text (attr, s);
		g_free (s);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	} else if (priv->agent_string) {
		attr = mimedir_attribute_new_with_name ("AGENT");
		mimedir_attribute_set_value_text (attr, priv->agent_string);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "text");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	} else if (priv->agent_uri) {
		attr = mimedir_attribute_new_with_name ("AGENT");
		mimedir_attribute_set_value_uri (attr, priv->agent_uri);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "uri");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Organization (3.5.5) */

	if (priv->organization) {
		list = NULL;

		for (list2 = priv->organization; list2 != NULL; list2 = g_slist_next (list2))
			list = g_slist_append (list, g_slist_append (NULL, list2->data));

		attr = mimedir_attribute_new_with_name ("ORG");
		mimedir_attribute_set_value_structured_text (attr, list);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));

		for (list2 = list; list2 != NULL; list2 = g_slist_next (list2))
			g_slist_free ((GSList *) list2->data);
		g_slist_free (list);
	}

	/*
	 * Explanatory Types (RFC 2426, Section 3.6)
	 */

	/* Categories (3.6.1) */

	if (priv->categories) {
		attr = mimedir_attribute_new_with_name ("CATEGORIES");
		mimedir_attribute_set_value_text_list (attr, priv->categories);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Note (3.6.2) */

	if (priv->note && priv->note[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("NOTE");
		mimedir_attribute_set_value_text (attr, priv->note);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Product ID (3.6.3) */

	attr = mimedir_attribute_new_with_name ("PRODID");
	mimedir_attribute_set_value_text (attr, "-//GNOME//NONSGML " PACKAGE_STRING "//EN");
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/* Revision (3.6.4) */

	g_assert (priv->revision != NULL);
	attr = mimedir_attribute_new_with_name ("REV");
	mimedir_attribute_set_value_datetime (attr, priv->revision);
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/* Sort string (3.6.5) */

	if (priv->sort_string && priv->sort_string[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("SORT-STRING");
		mimedir_attribute_set_value_text (attr, priv->sort_string);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Sound (3.6.6) */

	if (priv->sound) {
		attr = mimedir_attribute_new_with_name ("SOUND");
		mimedir_attribute_set_value_data (attr, priv->sound->str, priv->sound->len);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	} else if (priv->sound_uri) {
		attr = mimedir_attribute_new_with_name ("SOUND");
		mimedir_attribute_set_value_uri (attr, priv->sound_uri);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "uri");
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* UID (3.6.7) */

	if (priv->uid && priv->uid[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("UID");
		mimedir_attribute_set_value_text (attr, priv->uid);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* URL (3.6.8) */

	if (priv->url && priv->url[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("URL");
		mimedir_attribute_set_value (attr, priv->url);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Version (3.6.9) */

	attr = mimedir_attribute_new_with_name ("VERSION");
	mimedir_attribute_set_value (attr, "3.0");
	mimedir_profile_append_attribute (profile, attr);
	g_object_unref (G_OBJECT (attr));

	/*
	 * Security Types (RFC 2426, Section 3.7)
	 */

	/* Access Classification (3.7.1) */

	if (priv->klass && priv->klass[0] != '\0') {
		attr = mimedir_attribute_new_with_name ("CLASS");
		mimedir_attribute_set_value (attr, priv->klass);
		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	/* Key (3.7.2) */

	for (node = priv->keys; node != NULL; node = g_list_next (node)) {
		MIMEDirVCardKey *key = (MIMEDirVCardKey *) node->data;

		attr = mimedir_attribute_new_with_name ("KEY");
		mimedir_attribute_set_value_text (attr, key->key);
		mimedir_attribute_append_parameter_simple (attr, "VALUE", "text");

		switch (key->type) {
		case MIMEDIR_VCARD_KEY_X509:
			mimedir_attribute_append_parameter_simple (attr, "TYPE", "X509");
			break;
		case MIMEDIR_VCARD_KEY_PGP:
			mimedir_attribute_append_parameter_simple (attr, "TYPE", "PGP");
			break;
		default:
			break; /* dummy */
		}

		mimedir_profile_append_attribute (profile, attr);
		g_object_unref (G_OBJECT (attr));
	}

	priv->changed = FALSE;

	return profile;
}

/**
 * mimedir_vcard_write_to_channel:
 * @vcard: a vCard
 * @channel: I/O channel to save to
 * @error: error storage location or %NULL
 *
 * Saves the vCard object to the supplied I/O channel. If an error occurs
 * during the write, @error will be set and %FALSE will be returned.
 * Otherwise, %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_vcard_write_to_channel (MIMEDirVCard *vcard, GIOChannel *channel, GError **error)
{
	MIMEDirProfile *profile;

	g_return_val_if_fail (vcard != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	profile = mimedir_vcard_write_to_profile (vcard);

	if (!mimedir_profile_write_to_channel (profile, channel, error))
		return FALSE;

	g_object_unref (G_OBJECT (profile));

	return TRUE;
}

/**
 * mimedir_vcard_write_to_string:
 * @vcard: a vCard
 *
 * Saves the vCard object to a newly allocated memory buffer. You should
 * free the returned buffer with g_free().
 *
 * Return value: a newly allocated memory buffer
 **/
gchar *
mimedir_vcard_write_to_string (MIMEDirVCard *vcard)
{
	MIMEDirProfile *profile;
	gchar *s;

	g_return_val_if_fail (vcard != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), FALSE);

	profile = mimedir_vcard_write_to_profile (vcard);

	s = mimedir_profile_write_to_string (profile);

	g_object_unref (G_OBJECT (profile));

	return s;
}

/**
 * mimedir_vcard_set_birthday:
 * @vcard: a vCard
 * @birthday: a #MIMEDirDateTime object or %NULL
 *
 * Set's the person's birthday. Setting the birthday to %NULL unsets it.
 **/
void
mimedir_vcard_set_birthday (MIMEDirVCard *vcard, MIMEDirDateTime *birthday)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (birthday == NULL || MIMEDIR_IS_DATETIME (birthday));

	if (vcard->priv->birthday)
		g_object_unref (G_OBJECT (vcard->priv->birthday));
	vcard->priv->birthday = birthday;
	if (vcard->priv->birthday)
		g_object_ref (G_OBJECT (vcard->priv->birthday));
}

/**
 * mimedir_vcard_get_birthday:
 * @vcard: a vCard
 *
 * Get's the person's birthday. The return value is not ref'fed.
 *
 * Return value: a #MIMEDirDateTime object or %NULL if the birthday is undefined
 **/
MIMEDirDateTime *
mimedir_vcard_get_birthday (MIMEDirVCard *vcard)
{
	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	return vcard->priv->birthday;
}

/**
 * mimedir_vcard_append_address:
 * @vcard: a vCard
 * @address: a vCard address
 *
 * Appends the supplied address object to the vCard.
 **/
void
mimedir_vcard_append_address (MIMEDirVCard *vcard, MIMEDirVCardAddress *address)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (address != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address));

	mimedir_vcard_internal_append_address (vcard, address);

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_remove_address:
 * @vcard: a vCard
 * @address: a vCard address
 *
 * Removes the supplied address to the vCard.
 **/
void
mimedir_vcard_remove_address (MIMEDirVCard *vcard, MIMEDirVCardAddress *address)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (address != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_ADDRESS (address));

	mimedir_vcard_internal_remove_address (vcard, address);

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_append_email:
 * @vcard: a vCard
 * @email: a vCard e-mail
 *
 * Appends the supplied e-mail address to the vCard.
 **/
void
mimedir_vcard_append_email (MIMEDirVCard *vcard, MIMEDirVCardEMail *email)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (email != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (email));

	mimedir_vcard_internal_append_email (vcard, email);

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_remove_email:
 * @vcard: a vCard
 * @email: a vCard e-mail
 *
 * Removes the supplied e-mail address from the vCard.
 **/
void
mimedir_vcard_remove_email (MIMEDirVCard *vcard, MIMEDirVCardEMail *email)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (email != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_EMAIL (email));

	mimedir_vcard_internal_remove_email (vcard, email);

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_append_phone:
 * @vcard: a vCard
 * @phone: a vCard phone
 *
 * Appends the supplied telephone number to the vCard.
 **/
void
mimedir_vcard_append_phone (MIMEDirVCard *vcard, MIMEDirVCardPhone *phone)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (phone != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (phone));

	mimedir_vcard_internal_append_phone (vcard, phone);

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_remove_phone:
 * @vcard: a vCard
 * @phone: a vCard phone
 *
 * Removes the supplied telephone number from the vCard.
 **/
void
mimedir_vcard_remove_phone (MIMEDirVCard *vcard, MIMEDirVCardPhone *phone)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (phone != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD_PHONE (phone));

	mimedir_vcard_internal_remove_phone (vcard, phone);

	mimedir_vcard_changed (vcard);
}


/**
 * mimedir_vcard_get_preferred_address:
 * @vcard: a vCard
 *
 * Returns the preferred address of the supplied vCard object. If no address
 * has the preferred flag set, the first address of the address list is
 * returned. If the address list is empty, %NULL is returned.
 *
 * Return value: an address object or %NULL
 **/
MIMEDirVCardAddress *
mimedir_vcard_get_preferred_address (MIMEDirVCard *vcard)
{
	GSList *l;

	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	for (l = vcard->priv->addresses; l != NULL; l = g_slist_next (l)) {
		MIMEDirVCardAddress *add;
		gboolean pref;

		g_assert (MIMEDIR_IS_VCARD_ADDRESS (l->data));

		add = MIMEDIR_VCARD_ADDRESS (l->data);

		g_object_get (add, "preferred", &pref, NULL);

		if (pref)
			return add;
	}

	if (vcard->priv->addresses)
		return MIMEDIR_VCARD_ADDRESS (vcard->priv->addresses->data);

	return NULL;
}


/**
 * mimedir_vcard_get_preferred_email:
 * @vcard: a vCard
 *
 * Returns the preferred email address of the supplied vCard object. If no
 * email address has the preferred flag set, the first email address of the
 * email address list is returned. If the email address list is empty, %NULL
 * is returned.
 *
 * Return value: an email address object or %NULL
 **/
MIMEDirVCardEMail *
mimedir_vcard_get_preferred_email (MIMEDirVCard *vcard)
{
	GSList *l;

	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	for (l = vcard->priv->email; l != NULL; l = g_slist_next (l)) {
		MIMEDirVCardEMail *email;
		gboolean pref;

		g_assert (MIMEDIR_IS_VCARD_EMAIL (l->data));

		email = MIMEDIR_VCARD_EMAIL (l->data);

		g_object_get (email, "preferred", &pref, NULL);

		if (pref)
			return email;
	}

	if (vcard->priv->email)
		return MIMEDIR_VCARD_EMAIL (vcard->priv->email->data);

	return NULL;
}


/**
 * mimedir_vcard_get_preferred_phone:
 * @vcard: a vCard
 *
 * Returns the preferred phone number of the supplied vCard object. If no
 * phone number has the preferred flag set, the first phone number of the
 * phone number list is returned. If the phone number list is empty, %NULL
 * is returned.
 *
 * Return value: a phone number object or %NULL
 **/
MIMEDirVCardPhone *
mimedir_vcard_get_preferred_phone (MIMEDirVCard *vcard)
{
	GSList *l;

	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	for (l = vcard->priv->phone; l != NULL; l = g_slist_next (l)) {
		MIMEDirVCardPhone *phone;
		gboolean pref;

		g_assert (MIMEDIR_IS_VCARD_PHONE (l->data));

		phone = MIMEDIR_VCARD_PHONE (l->data);

		g_object_get (phone, "preferred", &pref, NULL);

		if (pref)
			return phone;
	}

	if (vcard->priv->phone)
		return MIMEDIR_VCARD_PHONE (vcard->priv->phone->data);

	return NULL;
}

/**
 * mimedir_vcard_set_timezone:
 * @vcard: a vCard
 * @timezone: time zone in minutes from UTC
 *
 * Sets the time zone to @timezone.
 **/
void
mimedir_vcard_set_timezone (MIMEDirVCard *vcard, gint timezone)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (timezone >= MIMEDIR_VCARD_TIMEZONE_MIN &&
			  timezone <= MIMEDIR_VCARD_TIMEZONE_MAX);

	vcard->priv->timezone = timezone;

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_clear_timezone:
 * @vcard: a vCard
 *
 * Clears the card's time zone information.
 **/
void
mimedir_vcard_clear_timezone (MIMEDirVCard *vcard)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));

	vcard->priv->timezone = G_MAXINT;

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_get_timezone:
 * @vcard: a vCard
 * @timezone: pointer to the timezone storage location or %NULL
 *
 * If the object's time zone is cleared, %FALSE is returned and @timezone is
 * left unchanged. Otherwise %TRUE is returned and @timezone is set to the
 * vCard's time zone.
 *
 * Return value: %TRUE if the object has a valid time zone, %FALSE otherwise
 **/
gboolean
mimedir_vcard_get_timezone (MIMEDirVCard *vcard, gint *timezone)
{
	g_return_val_if_fail (vcard != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), FALSE);

	if (vcard->priv->timezone < MIMEDIR_VCARD_TIMEZONE_MIN ||
	    vcard->priv->timezone > MIMEDIR_VCARD_TIMEZONE_MAX)
		return FALSE;

	if (timezone)
		*timezone = vcard->priv->timezone;

	return TRUE;
}

/**
 * mimedir_vcard_set_geo_position:
 * @vcard: a vCard
 * @latitude: new latitude
 * @longitude: new longitude
 *
 * Sets the vCard's geographical position to @latitude and @longitude.
 **/
void
mimedir_vcard_set_geo_position (MIMEDirVCard *vcard, gdouble latitude, gdouble longitude)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));
	g_return_if_fail (latitude >= MIMEDIR_VCARD_LATITUDE_MIN &&
			  latitude <= MIMEDIR_VCARD_LATITUDE_MAX);
	g_return_if_fail (longitude >= MIMEDIR_VCARD_LONGITUDE_MIN &&
			  longitude <= MIMEDIR_VCARD_LONGITUDE_MAX);

	vcard->priv->latitude  = latitude;
	vcard->priv->longitude = longitude;

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_clear_geo_position:
 * @vcard: a vCard
 *
 * Clears the vCard's geographical position.
 **/
void
mimedir_vcard_clear_geo_position (MIMEDirVCard *vcard)
{
	g_return_if_fail (vcard != NULL);
	g_return_if_fail (MIMEDIR_IS_VCARD (vcard));

	vcard->priv->latitude  = G_MAXDOUBLE;
	vcard->priv->longitude = G_MAXDOUBLE;

	mimedir_vcard_changed (vcard);
}

/**
 * mimedir_vcard_get_geo_position:
 * @vcard: a vCard
 * @latitude: pointer to the latitude storage location or %NULL
 * @longitude: pointer to the longitude storage location or %NULL
 *
 * If the object's geographical position is cleared, %FALSE is returned and
 * @latitude and @longitude are left unchanged. Otherwise %TRUE is returned
 * and @latitude and @longitude are set to the vCard's geographical position.
 *
 * Return value: %TRUE if the object has a valid geographical position,
 * %FALSE otherwise
 **/
gboolean
mimedir_vcard_get_geo_position (MIMEDirVCard *vcard, gdouble *latitude, gdouble *longitude)
{
	g_return_val_if_fail (vcard != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), FALSE);

	if (vcard->priv->latitude  < MIMEDIR_VCARD_LATITUDE_MIN  ||
	    vcard->priv->latitude  > MIMEDIR_VCARD_LATITUDE_MAX  ||
	    vcard->priv->longitude < MIMEDIR_VCARD_LONGITUDE_MIN ||
	    vcard->priv->longitude > MIMEDIR_VCARD_LONGITUDE_MAX)
		return FALSE;

	if (latitude)
		*latitude = vcard->priv->latitude;
	if (longitude)
		*longitude = vcard->priv->longitude;

	return TRUE;
}

/**
 * mimedir_vcard_get_as_string:
 * @vcard: a vCard
 *
 * Returns the contents of the vCard object as a multi-line human-readable
 * string. It is not guaranteed that this string is of a particular
 * format or contains all information from the vCard object. Nevertheless,
 * it will contain all vital information. You should free the returned
 * string with g_free().
 *
 * Return value: a multi-line string
 **/
gchar *
mimedir_vcard_get_as_string (MIMEDirVCard *vcard)
{
	MIMEDirVCardPriv *priv;
	GString *s;
	GSList *l;
	GList *node;

	g_return_val_if_fail (vcard != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_VCARD (vcard), NULL);

	priv = vcard->priv;

	s = g_string_new (NULL);

	/* Name */

	if (priv->fn && *priv->fn != '\0')
		g_string_append_printf (s, _("Name: %s\n"), priv->fn);
	else
		g_string_append (s, _("Name\n"));
	if (priv->prefix && *priv->prefix != '\0')
		g_string_append_printf (s, _("  Prefix:     %s\n"), priv->prefix);
	if (priv->givenname && *priv->givenname != '\0')
		g_string_append_printf (s, _("  Given:      %s\n"), priv->givenname);
	if (priv->middlename && *priv->middlename != '\0')
		g_string_append_printf (s, _("  Additional: %s\n"), priv->middlename);
	if (priv->familyname && *priv->familyname != '\0')
		g_string_append_printf (s, _("  Family:     %s\n"), priv->familyname);
	if (priv->suffix && *priv->suffix != '\0')
		g_string_append_printf (s, _("  Suffix:     %s\n"), priv->suffix);

        /* Birthday */

	if (priv->birthday) {
		g_string_append_c (s, '\n');
		g_string_append_printf (s, _("Birth Date: %04d-%02d-%02d\n"),
					priv->birthday->year,
					priv->birthday->month,
					priv->birthday->day);
	}

	/* Addresses */

	for (l = priv->addresses; l != NULL; l = g_slist_next (l)) {
		MIMEDirVCardAddress *address;
		gchar *full;

		g_assert (MIMEDIR_IS_VCARD_ADDRESS (l->data));

		address = MIMEDIR_VCARD_ADDRESS (l->data);

		g_string_append_c (s, '\n');

		g_object_get (G_OBJECT (address), "full", &full, NULL);

		if (full) {
			GString *str;
			gchar *tmp, *type;

			type = mimedir_vcard_address_get_type_string (address);
			g_string_append_printf (s, _("Address label (%s):\n"), type);
			g_free (type);

			str = g_string_new ("  ");
			for (tmp = full; tmp[0] != '\0'; tmp++) {
				if (tmp[0] == '\n')
					g_string_append (str, "\n  ");
				else
					g_string_append_c (str, tmp[0]);
			}
			g_string_append (s, str->str);
			g_string_free (str, TRUE);
			g_string_append_c (s, '\n');

			g_free (full);
		} else {
			gchar *pobox, *ext, *street, *loc, *region, *pcode, *country;
			gchar *type;

			type = mimedir_vcard_address_get_type_string (address);
			g_string_append_printf (s, _("Address (%s):\n"), type);
			g_free (type);

			g_object_get (G_OBJECT (address),
				      "full",     &full,
				      "pobox",    &pobox,
				      "extended", &ext,
				      "street",   &street,
				      "locality", &loc,
				      "region",   &region,
				      "pcode",    &pcode,
				      "country",  &country,
				      NULL);
			if (pobox && pobox[0] != '\0')
				g_string_append_printf (s, _("  Postal Box:  %s\n"), pobox);
			if (ext && ext[0] != '\0')
				g_string_append_printf (s, _("  Extended:    %s\n"), ext);
			if (street && street[0] != '\0')
				g_string_append_printf (s, _("  Street:      %s\n"), street);
			if (loc && loc[0] != '\0')
				g_string_append_printf (s, _("  City:        %s\n"), loc);
			if (region && region[0] != '\0')
				g_string_append_printf (s, _("  Region:      %s\n"), region);
			if (pcode && pcode[0] != '\0')
				g_string_append_printf (s, _("  Postal Code: %s\n"), pcode);
			if (country && country[0] != '\0')
				g_string_append_printf (s, _("  Country:     %s\n"), country);

			g_free (pobox);
			g_free (ext);
			g_free (street);
			g_free (loc);
			g_free (region);
			g_free (pcode);
			g_free (country);
		}
	}

	/* Phone */

	if (priv->phone) {
		g_string_append_c (s, '\n');

		if (priv->phone->next) {
			GSList *l;

			g_string_append (s, _("Telephone:\n"));

			for (l = priv->phone; l != NULL; l = g_slist_next (l)) {
				MIMEDirVCardPhone *phone;
				gchar *number, *type;

				g_assert (MIMEDIR_IS_VCARD_PHONE (l->data));

				phone = MIMEDIR_VCARD_PHONE (l->data);
				g_object_get (G_OBJECT (phone), "number", &number, NULL);
				type = mimedir_vcard_phone_get_type_string (phone);
				g_string_append_printf (s, "  %s (%s)\n", number, type);
				g_free (number);
				g_free (type);
			}
		} else {
			MIMEDirVCardPhone *phone;
			gchar *number, *type;

			g_assert (MIMEDIR_IS_VCARD_PHONE (priv->phone->data));

			phone = MIMEDIR_VCARD_PHONE (priv->phone->data);
			g_object_get (G_OBJECT (phone), "number", &number, NULL);
			type = mimedir_vcard_phone_get_type_string (phone);
			g_string_append_printf (s, _("Telephone: %s (%s)\n"), number, type);
			g_free (number);
			g_free (type);
		}
	}

	/* E-mail */

	if (priv->email) {
		g_string_append_c (s, '\n');

		if (priv->email->next) {
			GSList *l;

			g_string_append (s, _("E-mail:\n"));

			for (l = priv->email; l != NULL; l = g_slist_next (l)) {
				MIMEDirVCardEMail *email;
				gchar *address, *type;

				g_assert (MIMEDIR_IS_VCARD_EMAIL (l->data));

				email = MIMEDIR_VCARD_EMAIL (l->data);
				g_object_get (G_OBJECT (email), "address", &address, NULL);
				type = mimedir_vcard_email_get_type_string (email);
				g_string_append_printf (s, "  %s (%s)\n", address, type);
				g_free (address);
				g_free (type);
			}
		} else {
			MIMEDirVCardEMail *email;
			gchar *address, *type;

			g_assert (MIMEDIR_IS_VCARD_EMAIL (priv->email->data));

			email = MIMEDIR_VCARD_EMAIL (priv->email->data);
			g_object_get (G_OBJECT (email), "address", &address, NULL);
			type = mimedir_vcard_email_get_type_string (email);
			g_string_append_printf (s, _("E-mail: %s (%s)\n"), address, type);
			g_free (address);
			g_free (type);
		}
        }

	/* Mailer */

	if (priv->mailer)
		g_string_append_printf (s, "Mailer: %s\n", priv->mailer);

	/* Geographical */

	if (priv->timezone >= MIMEDIR_VCARD_TIMEZONE_MIN &&
	    priv->timezone <= MIMEDIR_VCARD_TIMEZONE_MAX) {
		gint hour, minute;
		hour   = priv->timezone / 60;
		minute = priv->timezone % 60;
		if (hour < 0)
			hour = -hour;
		if (minute < 0)
			minute = -minute;
		g_string_append_printf (s, _("Time zone: %c%02d:%02d\n"),
					priv->timezone < 0000 ? '-' : '+',
					hour, minute);
	}

	if (priv->latitude  >= MIMEDIR_VCARD_LATITUDE_MIN  &&
	    priv->latitude  <= MIMEDIR_VCARD_LATITUDE_MAX  &&
	    priv->longitude >= MIMEDIR_VCARD_LONGITUDE_MIN &&
	    priv->longitude <= MIMEDIR_VCARD_LONGITUDE_MAX)
	    g_string_append_printf (s, _("Location: %.2f,%.2f\n"),
				    priv->longitude, priv->latitude);

	/* Organization */

	if (priv->organization) {
		gchar *sl;

		sl = mimedir_utils_strcat_list (priv->organization, ", ");
		g_string_append_printf (s, _("Organization: %s\n"), sl);
		g_free (sl);
	}

	if (priv->jobtitle && priv->jobtitle[0] != '\0')
		g_string_append_printf (s, _("Job title: %s\n"), priv->jobtitle);

	if (priv->jobrole && priv->jobrole[0] != '\0')
		g_string_append_printf (s, _("Business role: %s\n"), priv->jobrole);

	/* Explanatory */

	g_string_append_c (s, '\n');

	if (priv->categories != NULL) {
		gchar *sl;

		sl = mimedir_utils_strcat_list (priv->categories, ", ");
		g_string_append_printf (s, _("Categories: %s\n"), sl);
		g_free (sl);
	}

	if (priv->note && priv->note[0] != '\0')
		g_string_append_printf (s, _("Comment: %s\n"), priv->note);

	if (priv->url && priv->url[0] != '\0')
		g_string_append_printf (s, _("Homepage: %s\n"), priv->url);

	if (priv->uid && priv->uid[0] != '\0')
		g_string_append_printf (s, _("Unique string: %s\n"), priv->uid);

	/* Security */

	for (node = priv->keys; node != NULL; node = g_list_next (node)) {
		MIMEDirVCardKey *key = (MIMEDirVCardKey *) node->data;
		const gchar *type;

		switch (key->type) {
		case MIMEDIR_VCARD_KEY_X509:
			type = _("X.509");
			break;
		case MIMEDIR_VCARD_KEY_PGP:
			type = _("PGP");
			break;
		default:
			type = _("yes");
			break;
		}

		g_string_append_printf (s, _("Public Key: %s\n"), type);
        }
        
	return g_string_free (s, FALSE);
}
