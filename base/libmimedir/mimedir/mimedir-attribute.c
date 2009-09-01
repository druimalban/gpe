/* RFC 2425 MIME Directory Attribute Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attribute.c 257 2006-08-15 21:16:18Z srittau $
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

#include <string.h>
#include <time.h>

#include <glib.h>

#include "mimedir-attribute.h"
#include "mimedir-utils.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


enum {
	PROP_NAME = 1,
	PROP_GROUP
};

typedef struct _MIMEDirParameter {
	gchar  *name;
	GSList *values;
} MIMEDirParameter;


static void mimedir_attribute_class_init	(MIMEDirAttributeClass	*klass);
static void mimedir_attribute_init		(MIMEDirAttribute	*attribute);
static void mimedir_attribute_dispose		(GObject		*object);
static void mimedir_attribute_set_property	(GObject		*object,
						 guint			 property_id,
						 const GValue		*value,
						 GParamSpec		*pspec);
static void mimedir_attribute_get_property	(GObject		*object,
						 guint			 property_id,
						 GValue			*value,
						 GParamSpec		*pspec);

static void mimedir_attribute_free_parameter	(MIMEDirParameter *parameter);


struct _MIMEDirAttributePriv {
	MIMEDirAttributeType     type;
	MIMEDirAttributeEncoding encoding;
	gchar   *charset;
	gchar	*name;
	gchar	*group;
	GString	*value;
	GSList	*parameters;
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_attribute_get_type (void)
{
	static GType mimedir_attribute_type = 0;

	if (!mimedir_attribute_type) {
		static const GTypeInfo mimedir_attribute_info = {
			sizeof (MIMEDirAttributeClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_attribute_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirAttribute),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_attribute_init,
		};

		mimedir_attribute_type = g_type_register_static (G_TYPE_OBJECT,
								 "MIMEDirAttribute",
								 &mimedir_attribute_info,
								 0);
	}

	return mimedir_attribute_type;
}


static void
mimedir_attribute_class_init (MIMEDirAttributeClass *klass)
{
	GObjectClass *gobject_class;
        GParamSpec *pspec;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_attribute_dispose;
	gobject_class->set_property = mimedir_attribute_set_property;
	gobject_class->get_property = mimedir_attribute_get_property;

	parent_class = g_type_class_peek_parent (klass);

	/* Properties */

	pspec = g_param_spec_string ("name",
				     _("Name"),
				     _("The attribute's name"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_NAME, pspec);
	pspec = g_param_spec_string ("group",
				     _("Group"),
				     _("The attribute's group"),
				     NULL,
				     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class, PROP_GROUP, pspec);
}


static void
mimedir_attribute_init (MIMEDirAttribute *attribute)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

        attribute->priv = g_new0 (MIMEDirAttributePriv, 1);

	attribute->priv->charset = g_strdup ("utf-8");

	attribute->priv->type  = MIMEDIR_ATTRIBUTE_TYPE_UNKNOWN;
	attribute->priv->value = g_string_new (NULL);
}


static void
mimedir_attribute_dispose (GObject *object)
{
	MIMEDirAttribute *attribute;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (object));

	attribute = MIMEDIR_ATTRIBUTE (object);

	if (attribute->priv) {
		GSList *list;

		g_free (attribute->priv->charset);
		g_free (attribute->priv->group);
		g_free (attribute->priv->name);
		g_string_free (attribute->priv->value, TRUE);

		for (list = attribute->priv->parameters; list != NULL; list = g_slist_next (list))
			mimedir_attribute_free_parameter ((MIMEDirParameter *) list->data);
		g_slist_free (attribute->priv->parameters);

		g_free (attribute->priv);
	}

	G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
mimedir_attribute_set_property_string (gchar **dest, const GValue *value)
{
	const gchar *string;

	string = g_value_get_string (value);
	g_free (*dest);
	*dest = g_strdup (string);
}


static void
mimedir_attribute_set_property (GObject		*object,
				guint		 property_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	MIMEDirAttributePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (object));

	priv = MIMEDIR_ATTRIBUTE (object)->priv;

	switch (property_id) {
	case PROP_NAME: {
		const gchar *s;
		s = g_value_get_string (value);
		if (!mimedir_utils_is_token (s)) {
			g_critical ("invalid name");
			return;
		}
		mimedir_attribute_set_property_string (&priv->name, value);
		break;
	}
	case PROP_GROUP: {
		const gchar *s;
		s = g_value_get_string (value);
		if (!mimedir_utils_is_token (s)) {
			g_critical ("invalid group name");
			return;
		}
		mimedir_attribute_set_property_string (&priv->group, value);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}


static void
mimedir_attribute_get_property (GObject		*object,
				guint		 property_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	MIMEDirAttributePriv *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (object));

	priv = MIMEDIR_ATTRIBUTE (object)->priv;

	switch (property_id) {
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_GROUP:
		g_value_set_string (value, priv->group);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		return;
	}
}

/*
 * Coding Functions
 */

#if 0
/* Convert an integer value in the range 0 <= i <= 63 to the appropriate
 * Base64 character.
 */
static gchar
base64_int_to_char (guint i)
{
	g_assert (i <= 63);

	if (i <= 25)
		return i + 'A';
	else if (i <= 51)
		return i + 'a' - 26;
	else if (i <= 61)
		return i + '0' - 52;
	else if (i == 62)
		return '+';
	else /* i == 63 */
		return '/';
}
#endif

/* Convert a Base64 character to the appropriate int value 0 <= i <= 63. If
 * the supplied character is the Base64 fill character '=', return 64. If
 * the supplied character is not a valid Base64 character, return 65.
 */
static guint
base64_char_to_int (gchar c)
{
	if (c >= 'A' && c <= 'Z')
		return c - 'A';
	else if (c >= 'a' && c <= 'z')
		return c - 'a' + 26;
	else if (c >= '0' && c <= '9')
		return c - '0' + 52;
	else if (c == '+')
		return 62;
	else if (c == '/')
		return 63;
	else if (c == '=')
		return 64;

	return 65;
}

/* This is an ad-hoc check for Base64 encoding. It takes a
 * string as input, which is supposed to be the first line of an
 * attribute. If this line contains either the property ENCODING=b
 * or the property ENCODING=BASE64, this function returns TRUE,
 * otherwise it returns FALSE.
 *
 * This hack is needed, because Base64 encoded attributes break the normal
 * line folding mechanism.
 */
static gboolean
base64_check (const gchar *line)
{
	GString *token;
	guint mode;
	gboolean res;

	token = g_string_new ("");

	mode = 0; /* 0: parsing attribute name; 1: parsing property */
	for (; *line != '\0'; line++) {
		if (*line == ':')
			break;
		else if (*line == ';') {
			if (mode == 1) {
				if (!g_ascii_strcasecmp (token->str, "ENCODING=b") ||
				    !g_ascii_strcasecmp (token->str, "ENCODING=BASE64")) {
					g_string_free (token, TRUE);
					return TRUE;
				}
				g_string_free (token, TRUE);
				token = g_string_new ("");
			} else
				mode = 1;
		} else if (mode == 1)
			g_string_append_c (token, *line);
	}

	res = g_ascii_strcasecmp (token->str, "ENCODING=b") == 0 ||
	      g_ascii_strcasecmp (token->str, "ENCODING=BASE64") == 0;

	g_string_free (token, TRUE);

	return res;
}

#if 0
static gchar *
base64_encode (const gchar *string, GError **error)
{
	GString *encoded;
	const gchar *buffer = string;
	gsize size = strlen (string);

	encoded = g_string_sized_new (size * 4 / 3 + 4);

	for (; size > 0; size -= size > 3 ? 3 : size, buffer += 3) {
		guint8 s1, s2, s3;
		guint c1, c2, c3, c4;

		s1 = (guint8) buffer[0];
		s2 = (guint8) size >= 2 ? buffer[1] : 0;
		s3 = (guint8) size >= 3 ? buffer[2] : 0;

		c1 = s1 >> 2;
		c2 = ((s1 % 0x04) << 4) + (s2 >> 4);
		c3 = ((s2 % 0x10) << 2) + (s3 >> 6);
		c4 = s3 % 0x40;

		g_string_append_c (encoded, base64_int_to_char (c1));
		g_string_append_c (encoded, base64_int_to_char (c2));
		g_string_append_c (encoded, size >= 2 ? base64_int_to_char (c3) : '=');
		g_string_append_c (encoded, size >= 3 ? base64_int_to_char (c4) : '=');
	}

	return g_string_free (encoded, FALSE);
}
#endif

/* Decode the supplied Base64 string. Return a newly allocated buffer that
 * must be freed by the called using g_free(). If there are errors while
 * decoding the string, return NULL and set error accordingly.
 */
static gchar *
base64_decode (const gchar *string, GError **error)
{
	GString *decoded;
	const gchar *buffer = string;
	gsize size = strlen (string);

	decoded = g_string_sized_new (size * 3 / 4 + 3);

	for (; size >= 4; size -= 4, buffer += 4) {
		int i;
		guint is[4]; /* decoded characters */
		guint8 c1, c2, c3;

		for (i = 0; i < 4; i++)
			is[i] = base64_char_to_int (buffer[i]);

		/* The first two characters of each four character block
		 * must not be '='.
		 */
		if (is[0] > 63 || is[1] > 63 || is[2] > 64 || is[3] > 64) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_BASE64, MIMEDIR_ATTRIBUTE_ERROR_INVALID_BASE64_STR);
			g_string_free (decoded, TRUE);
			return NULL;
		}

		c1 = (is[0] << 2) + (is[1] >> 4);
		c2 = ((is[1] % 0x10) << 4) + (is[2] == 0x40 ? 0 : (is[2] >> 2));
		c3 = (is[2] == 0x40 ? 0 : ((is[2] % 4) << 6)) + (is[3] == 0x40 ? 0 : is[3]);

		g_string_append_c (decoded, c1);
		if (is[2] != 64)
			g_string_append_c (decoded, c2);
		if (is[2] != 64 && is[3] != 64)
			g_string_append_c (decoded, c3);
	}

	return g_string_free (decoded, FALSE);
}

/* This is an ad-hoc check for quoted-printable encoding. It takes a
 * string as input, which is supposed to be the first line of an
 * attribute. If this line contains either the property QUOTED-PRINTABLE
 * or the property ENCODING=QUOTED-PRINTABLE, this function returns TRUE,
 * otherwise it returns FALSE.
 *
 * This hack is needed, because QP encoded attributes break the normal
 * line folding mechanism.
 */
static gboolean
qp_check (const gchar *line)
{
	GString *token;
	guint mode;
	gboolean res;

	token = g_string_new ("");

	mode = 0;
	for (; *line != '\0'; line++) {
		if (*line == ':')
			break;
		else if (*line == ';') {
			if (mode == 1) {
				if (!g_ascii_strcasecmp (token->str, "QUOTED-PRINTABLE") ||
				    !g_ascii_strcasecmp (token->str, "ENCODING=QUOTED-PRINTABLE")) {
					g_string_free (token, TRUE);
					return TRUE;
				}
				g_string_free (token, TRUE);
				token = g_string_new ("");
			} else
				mode = 1;
		} else if (mode == 1)
			g_string_append_c (token, *line);
	}

	res = g_ascii_strcasecmp (token->str, "QUOTED-PRINTABLE") == 0 ||
	      g_ascii_strcasecmp (token->str, "ENCODING=QUOTED-PRINTABLE") == 0;

	g_string_free (token, TRUE);

	return res;
}

/* This function is a hack. It assumes that the QP-coded string is in the
 * current locale and converts it to UTF-8. Also it strips all carriage
 * returns from the string to streamline it. This means that it is not
 * possible to use QP to encode binary data. (It's not thought for that,
 * so I hope nobody had that idea.)
 */
static gchar *
qp_decode (const gchar *string, GError **error)
{
	GString *s;
	const gchar *text;

	text = string;

	s = g_string_new (NULL);

	while (text[0] != '\0') {
		gchar c;

		if (text[0] == '=') {
			text++;

			if (text[0] >= 'A' && text[0] <= 'F')
				c = (text[0] - 'A' + 10) << 4;
			else if (text[0] >= 'a' && text[0] <= 'f')
				c = (text[0] - 'a' + 10) << 4;
			else if (text[0] >= '0' && text[0] <= '9')
				c = (text[0] - '0') << 4;
			else {
				g_string_free (s, TRUE);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP, MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP_STR);
				return NULL;
			}

			text++;

			if (text[0] >= 'A' && text[0] <= 'F')
				c |= text[0] - 'A' + 10;
			else if (text[0] >= 'a' && text[0] <= 'f')
				c |= text[0] - 'a' + 10;
			else if (text[0] >= '0' && text[0] <= '9')
				c |= text[0] - '0';
			else {
				g_string_free (s, TRUE);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP, MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP_STR);
				return NULL;
			}

			text++;

		} else {
			c = text[0];
			text++;
		}

		if (c != '\r')
			g_string_append_c (s, c);
	}

	return g_string_free (s, FALSE);
}

/*
 * Utility Functions
 */

static void
mimedir_attribute_free_parameter (MIMEDirParameter *parameter)
{
	g_free (parameter->name);
	g_free (parameter);
}


static gchar *
escape (const gchar *text, GError **error)
{
	GString *string;
	gunichar uc;

	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	string = g_string_new ("");

	uc = g_utf8_get_char(text);
	while (uc != '\0') {
		switch (uc) {
		case ';':
			g_string_append (string, "\\;");
			break;
		case ',':
			g_string_append (string, "\\,");
			break;
		case '\\':
			g_string_append (string, "\\\\");
			break;
		case '\r':
			/* ignore */
			break;
		case '\n':
			g_string_append (string, "\\n");
			break;
		default:
		  if (uc != '\t' && (! g_unichar_isprint(uc))) {
		    g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR, uc, "text");
		    return FALSE;
		  }
		  g_string_append_unichar (string, uc);
		  break;
		}

		text = g_utf8_next_char(text);
		uc = g_utf8_get_char(text);
	}

	return g_string_free (string, FALSE);
}


static gchar *
unescape (const gchar *text, GError **error)
{
	GString *string;
	gunichar uc;

	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	string = g_string_new ("");

	uc = g_utf8_get_char(text);
	while (uc != '\0') {
		switch (uc) {
		case '\\':
			text = g_utf8_next_char(text);
			uc = g_utf8_get_char(text);
			switch (uc) {
			case 'n':
			case 'N':
				g_string_append_c (string, '\n');
				break;
			default:
				if (uc != '\t' && (! g_unichar_isprint(uc))) {
					g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR, uc, "text");
					return NULL;
				}
				g_string_append_unichar (string, uc);
				break;
			}
			break;
		case '\r':
			/* ignore */
			break;
		case '\n':
			g_string_append_unichar (string, uc);
			break;
		default:
			if (uc != '\t' && (! g_unichar_isprint(uc))) {
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR, uc, "text");
				return NULL;
			}
			g_string_append_unichar (string, uc);
			break;
		}

		text = g_utf8_next_char(text);
		uc = g_utf8_get_char(text);
	}

	return g_string_free (string, FALSE);
}

static GSList *
split_quoted_string (const gchar *s, gchar c)
{
	GSList *list = NULL;
	GString *str = g_string_new ("");

	while (*s != '\0') {
		if (*s == c) {
			list = g_slist_prepend (list, g_string_free (str, FALSE));
			str = g_string_new ("");
		} else {
			g_string_append_c (str, *s);
		}
		s++;
	}
	list = g_slist_prepend (list, g_string_free (str, FALSE));

	list = g_slist_reverse (list);

	return list;
}

/*
 * Private Methods
 */

static MIMEDirParameter *
mimedir_attribute_get_parameter (MIMEDirAttribute *attribute, const gchar *name)
{
	GSList *list;

	for (list = attribute->priv->parameters; list != NULL; list = g_slist_next (list)) {
		MIMEDirParameter *parameter;

		parameter = (MIMEDirParameter *) list->data;
		if (g_ascii_strcasecmp (parameter->name, name) == 0)
			return parameter;
	}

	return NULL;
}


static gboolean
mimedir_attribute_internal_append_parameter (MIMEDirAttribute *attribute, const gchar *name, GSList *values, GError **error)
{
	MIMEDirParameter *parameter;
	GSList *l;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (!g_ascii_strcasecmp (name, "encoding")) {
		if (g_slist_length (values) != 1) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE, MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE_STR, name, attribute->priv->name);
			return FALSE;
		}
		if (!g_ascii_strcasecmp ((const gchar *) values->data, "b") ||
		    !g_ascii_strcasecmp ((const gchar *) values->data, "base64"))
			attribute->priv->encoding = MIMEDIR_ATTRIBUTE_ENCODING_BASE64;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "quoted-printable"))
			attribute->priv->encoding = MIMEDIR_ATTRIBUTE_ENCODING_QP;
	} else if (!g_ascii_strcasecmp (name, "value")) {
		if (g_slist_length (values) != 1) {
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE, MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE_STR, name, attribute->priv->name);
			return FALSE;
		}
		if (!g_ascii_strcasecmp ((const gchar *) values->data, "uri"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_URI;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "text"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_TEXT;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "date"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_DATE;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "time"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_TIME;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "date-time"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_DATETIME;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "integer"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_INTEGER;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "boolean"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_BOOLEAN;
		else if (!g_ascii_strcasecmp ((const gchar *) values->data, "float"))
			attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_FLOAT;
	}

	parameter = mimedir_attribute_get_parameter (attribute, name);
	if (!parameter) {
		parameter = g_new0 (MIMEDirParameter, 1);

		parameter->name = g_strdup (name);
		attribute->priv->parameters = g_slist_append (attribute->priv->parameters, parameter);
	}

	for (l = values; l != NULL; l = g_slist_next (l)) {
		const gchar *s = (const gchar *) l->data;
		if (!mimedir_utils_is_qsafe (s)) {
			g_critical ("invalid character in parameter value");
			return FALSE;
		}
		parameter->values = g_slist_append (parameter->values, g_strdup (s));
	}

	return TRUE;
}

/*
 * Parsing
 */
static inline gboolean accept_tokenchar(const gunichar uc, const gchar *full_line)
{
  /* Many implementations do not follow the rules for tokens.
     So, we accept almost anything in a token but display a warning if it
     isn't a valid token character */

  if ( uc == '\0' || (!g_unichar_isprint(uc)) || uc == '=' || uc == ':' || uc == ';' || uc == '.' )
    return FALSE;

  if (!mimedir_utils_is_token_char(uc))
    g_message("Ignoring invalid token character '%lc' in line %s", uc, full_line);

  return TRUE;
}

static gchar *
get_next_token (const gchar *line, gsize *len)
{
	GString *string;
	const gchar *full_line = line;

	string = g_string_new (NULL);

	while (accept_tokenchar(g_utf8_get_char(line), full_line)) {
		g_string_append_unichar (string, g_utf8_get_char(line));
		line = g_utf8_next_char(line);
	}

	if (len) *len = string->len;

	if (!string->str || string->str[0] == '\0') {
		g_string_free (string, TRUE);
		return NULL;
	}

	return g_string_free (string, FALSE);
}


static gchar *
decode_attribute_value (MIMEDirAttribute *attribute, const gchar *string, GError **error)
{
	gchar *s, *str;

	switch (attribute->priv->encoding) {
	case MIMEDIR_ATTRIBUTE_ENCODING_NONE:
	  /* If no encoding, data is already in UTF-8 */
		s = g_strdup (string);
		return s;
		break;
	case MIMEDIR_ATTRIBUTE_ENCODING_BASE64:
		s = base64_decode (string, error);
		if (!s)
			return NULL;

		break;
	case MIMEDIR_ATTRIBUTE_ENCODING_QP:
		s = qp_decode (string, error);
		if (!s)
			return NULL;

		break;
	default:
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_UNKNOWN_ENCODING, MIMEDIR_ATTRIBUTE_ERROR_UNKNOWN_ENCODING_STR, attribute->priv->name);
		return NULL;
	}

	/* If a decode has happened, we need to re-convert the data to UTF-8 
	   from whatever the original file format was */

	g_assert (s != NULL);

	str = g_convert (s, strlen (s), "utf-8", attribute->priv->charset, NULL, NULL, error);
	g_free (s);

	return str;
}

static ssize_t
get_line (const gchar *string, GString *dst)
{
	const gchar *end;
	gboolean empty;
	ssize_t pos = 0;

 	do {
 		const gchar *end_n, *end_r;
 
		end_n = strchr (string + pos, '\n');
		end_r = strchr (string + pos, '\r');

		if (end_r != NULL && end_r < end_n)
			end = end_r;
		else
			end = end_n;

		if (end) {
			ssize_t len;

			len = (ssize_t) (end - string - pos);
			if (len == 0) {
				empty = TRUE;
			} else {
				g_string_append_len (dst, string + pos, len);
				pos += len;
				empty = FALSE;
			}

			if (string[pos] == '\r' && string[pos + 1] == '\n')
				pos++;
			pos++;
		} else {
			g_string_append (dst, string + pos);
			pos = strlen (string);
			empty = FALSE;
		}
	} while (empty);

	return pos;
}

static ssize_t
get_attribute_string (const gchar *string, GString *gstring)
{
	ssize_t pos;
	gboolean qp;
	gboolean basesixfour;

	g_string_truncate (gstring, 0);

	/* Retrieve initial line */

	pos = get_line (string, gstring);

	qp = qp_check (gstring->str);
	basesixfour = base64_check (gstring->str);

	/* Retrieve folded lines */

	for (;;) {
		if (qp) {
			g_assert (gstring->len >= 1);

			/* Some creators just use QP line folding rules, 
			   others mix QP line folding with our normal line
			   folding so try to support either.

			   Note, this does not break conformant QP implementations
			   but strange things can happen if a non-conformant
			   implementation breaks a line after the = of an =XX sequence.  */

			if (gstring->str[gstring->len - 1] == '=') {

			  /* QP line folding */
			  gstring->str[gstring->len - 1] = '\0';
			  gstring->len--;

			  if (string[pos] == ' ' || string[pos] == '\t')
			    pos++;

			} else {

			  /* Not QP line folding, let's try standard line folding */

			  /* If next byte is not a white space, we're done */
			  if (string[pos] != ' ' && string[pos] != '\t')
			    break;
			  pos++;

			}
		} else if (basesixfour) {
			if (string[pos] == ' ' || string[pos] == '\t')
				pos++;
			else if (string[pos] == '\r' || string[pos] == '\n')
				break;
			else if (string[pos] == '\0')
				break;
		} else {

			/* If next byte is not a white space, we're done */

			if (string[pos] != ' ' && string[pos] != '\t')
				break;
			pos++;
		}

		pos += get_line (string + pos, gstring);
	}

	return pos;
}

static GSList *
parse_parameter_list (const gchar *string, gsize *_pos, gchar terminator, gboolean no_name_allowed, GError **error)
{
        GSList *list = NULL;
	const gchar *p = string + *_pos;
	gunichar uc;
	const gchar *end;

	if (! g_utf8_validate(p, -1, NULL)) {
	  g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("invalid UTF-8 data"), string);
	  return NULL;
	}

	if (g_utf8_get_char(p) == terminator)
	        return NULL;

        do {
	        gchar *param_name, *param_value = NULL;
		gsize len;

		/* Read parameter name */

		param_name = get_next_token (p, &len);

		if (len == 0) {
			g_free (param_name);
			mimedir_attribute_free_string_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("missing parameter name"), string);
			return NULL;
		}

		/* Read parameter value list */

		p = g_utf8_offset_to_pointer(p, len);
		uc = g_utf8_get_char(p);
		if (uc == '=') {
			GString *s = g_string_new (NULL);

			p = g_utf8_next_char(p);
			uc = g_utf8_get_char(p);

			if (uc == '"') {
			        p = g_utf8_next_char(p);
				uc = g_utf8_get_char(p);
				while (uc == '\t' ||
				       (g_unichar_isprint(uc) && uc != '"')) {
				  g_string_append_unichar (s, uc);
				  p = g_utf8_next_char(p);
				  uc = g_utf8_get_char(p);
				}
				if (uc != '"') {
					g_string_free (s, TRUE);
					g_free (param_name);
					mimedir_attribute_free_string_list (list);
					g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("missing closing quotation mark"), string);
					return NULL;
				}
			        p = g_utf8_next_char(p);
				uc = g_utf8_get_char(p);
			} else {
			        while (uc == '\t' ||
				       (g_unichar_isprint(uc) && uc != '"' && uc != ';')) {
				  if (uc == terminator)
				    break;
				  g_string_append_unichar (s, uc);
				  p = g_utf8_next_char(p);
				  uc = g_utf8_get_char(p);
				}
			}

			param_value = g_string_free (s, FALSE);
		}

		if (uc != terminator && uc != ';') {
			g_free (param_name);
			mimedir_attribute_free_string_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("wrong parameter syntax"), string);
			return NULL;
		}
		p = g_utf8_next_char(p);

		/* Append parameter to list */

		if (param_value == NULL) {
			if (!no_name_allowed) {
				g_free (param_name);
				mimedir_attribute_free_string_list (list);
				 g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("no parameter value"), string);
				 return NULL;
			}
			param_value = param_name;
			param_name = NULL;
		}

		list = g_slist_prepend (list, param_name);
		list = g_slist_prepend (list, param_value);
	} while (uc != terminator);

	p = g_utf8_prev_char(p);

	list = g_slist_reverse (list);
	*_pos = p - string;

	g_assert (g_slist_length (list) % 2 == 0);

	return list;
}

static gboolean
parse_attribute_parameters (MIMEDirAttribute *attr, const gchar *string, gsize *_pos, GError **error)
{
        GSList *list, *current;
	GError *err = NULL;
	gsize pos = *_pos;

	if (string[pos] != ';')
	      return TRUE;

	pos++;

	list = parse_parameter_list (string, &pos, ':', TRUE, &err);
	if (err) {
		g_propagate_error (error, err);
		return FALSE;
	}

	for (current = list; current != NULL; current = g_slist_next (current)) {
		const gchar *param_name, *param_value;
		GSList *param_list = NULL;

		param_name = (const gchar *) current->data;
		current = g_slist_next (current);
		g_assert (current != NULL);
		param_value = (const gchar *) current->data;

		if (param_value[0] != '\0')
			param_list = split_quoted_string (param_value, ',');

		/* Backwards compatibility: Handle parameters without name. */

		if (param_name == NULL) {
			param_list = g_slist_append (param_list, g_strdup (param_value));

			/* If the parameter name is QUOTED-PRINTABLE,
			 * the real parameter name is ENCODING.
			 */

			if (!g_ascii_strcasecmp (param_value, "QUOTED-PRINTABLE"))
				param_name = g_strdup ("ENCODING");

			/* Otherwise we assume that the name is TYPE. */

			else
				param_name = g_strdup ("TYPE");
		}

		mimedir_attribute_internal_append_parameter (attr, param_name, param_list, &err);
	}

	mimedir_attribute_free_string_list (list);

	*_pos = pos;

	return TRUE;
}

static gboolean
parse_attribute (MIMEDirAttribute *attr, const gchar *string, GError **error)
{
	MIMEDirAttributePriv *priv = attr->priv;
	gchar *token;
	const gchar *p = string;
	gsize len, pos;
	gunichar uc;

	if (! g_utf8_validate(p, -1, NULL)) {
	  g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("invalid UTF-8 data"), string);
	  return FALSE;
	}

	/* Read group/name */

	token = get_next_token (p, &len);
	if (len == 0) {
		g_free (token);
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("missing attribute name/group"), string);
		return FALSE;
	}

	p = g_utf8_offset_to_pointer(p, len);
	uc = g_utf8_get_char(p);

	g_free (priv->group);
	priv->group = NULL;

	if (uc == '.') {
		priv->group = token;
		p = g_utf8_next_char(p);
		uc = g_utf8_get_char(p);

		token = get_next_token (p, &len);
		if (len == 0) {
			g_free (token);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("no attribute name after group"), string);
			return FALSE;
		}

		p = g_utf8_offset_to_pointer(p, len);
		uc = g_utf8_get_char(p);
	}

	g_free (priv->name);
	priv->name = token;

	/* Read parameters */

	pos = p - string;
	if (!parse_attribute_parameters (attr, string, &pos, error))
	        return FALSE;
	p = string + pos;
	uc = g_utf8_get_char(p);

	/* Read value */

	if (uc != ':') {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("missing value delimiter"), string);
		return FALSE;
	}
	p = g_utf8_next_char(p);
	uc = g_utf8_get_char(p);

	{
		GString *s;
		gchar *str;

		s = g_string_new (NULL);

		while (uc != '\0') {
			if (uc != '\t' && (! g_unichar_isprint(uc))) {
				g_string_free (s, TRUE);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("invalid character"), string);
				return FALSE;
			}

			g_string_append_unichar (s, uc);
			p = g_utf8_next_char(p);
			uc = g_utf8_get_char(p);
		}

		/* Decode value */

		str = decode_attribute_value (attr, s->str, error);
		if (!str) {
			g_string_free (s, TRUE);
			return FALSE;
		}

		g_string_free (priv->value, TRUE);
		g_string_assign (s, str);
		priv->value = s;
		g_free (str);
	}

	return TRUE;
}

/*
 * Public Methods
 */

GQuark
mimedir_attribute_error_quark (void)
{
	static gchar qs[] = "mimedir-attribute-error-quark";

	return g_quark_from_static_string (qs);
}

/**
 * mimedir_attribute_new:
 *
 * Create a new #MIMEDirAttribute object.
 *
 * Return value: the new #MIMEDirAttribute object
 **/
MIMEDirAttribute *
mimedir_attribute_new (void)
{
	return g_object_new (MIMEDIR_TYPE_ATTRIBUTE, NULL);
}

/**
 * mimedir_attribute_new_with_name:
 * @name: attribute name
 *
 * Create a new #MIMEDirAttribute object with the supplied attribute name.
 *
 * Return value: the new #MIMEDirAttribute object
 **/
MIMEDirAttribute *
mimedir_attribute_new_with_name (const gchar *name)
{
	MIMEDirAttribute *attribute;

	g_return_val_if_fail (name != NULL && mimedir_utils_is_token (name), NULL);

	attribute = g_object_new (MIMEDIR_TYPE_ATTRIBUTE, NULL);

	g_free (attribute->priv->name);
	attribute->priv->name = g_strdup (name);

	return attribute;
}

/**
 * mimedir_attribute_get_name:
 * @attribute: a #MIMEDirAttribute
 *
 * Returns the name of the attribute.
 *
 * Return value: the attribute name
 **/
const gchar *
mimedir_attribute_get_name (MIMEDirAttribute *attribute)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);

	return attribute->priv->name;
}

/**
 * mimedir_attribute_set_group:
 * @attribute: a #MIMEDirAttribute
 * @group: new group name or %NULL
 *
 * Sets the name of the attribute to @group. If @group is %NULL the
 * attribute will not belong to any group.
 **/
void
mimedir_attribute_set_group (MIMEDirAttribute *attribute, const gchar *group)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (group == NULL || mimedir_utils_is_token (group));

	g_free (attribute->priv->group);
	if (group)
		attribute->priv->group = g_strdup (group);
	else
		attribute->priv->group = NULL;
}

/**
 * mimedir_attribute_get_group:
 * @attribute: a #MIMEDirAttribute
 *
 * Returns the group of the attribute. A %NULL value means that the attribute
 * does not belong to a group.
 *
 * Return value: the attribute group or %NULL
 **/
const gchar *
mimedir_attribute_get_group (MIMEDirAttribute *attribute)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);

	return attribute->priv->group;
}

/**
 * mimedir_attribute_set_charset:
 * @attribute: a #MIMEDirAttribute
 * @charset: character set for encoding/decoding this attribute
 *
 * Sets the character set that should be used to encode or decode this
 * attribute. Defaults to "utf-8".
 */
void
mimedir_attribute_set_charset (MIMEDirAttribute *attribute, const gchar *charset)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (charset != NULL);

	g_free (attribute->priv->charset);
	attribute->priv->charset = g_strdup (charset);
}

/**
 * mimedir_attribute_get_charset:
 * @attribute: a #MIMEDirAttribute
 *
 * Returns the character set that is used to encode or decode this attribute.
 *
 * Return value: the character set for encoding/decoding this attribute
 */
const gchar *
mimedir_attribute_get_charset (MIMEDirAttribute *attribute)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);

	return attribute->priv->charset;
}

/**
 * mimedir_attribute_append_parameter:
 * @attribute: a #MIMEDirAttribute
 * @name: parameter name
 * @values: list of parameter values
 *
 * Appends a parameter to an attribute. If a parameter with the same
 * name does already exist, the values are appended to it.
 **/
void
mimedir_attribute_append_parameter (MIMEDirAttribute *attribute, const gchar *name, GSList *values)
{
	GError *error = NULL;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (name != NULL && mimedir_utils_is_token (name));
	g_return_if_fail (values != NULL); /* there must be at least one value */

	mimedir_attribute_internal_append_parameter (attribute, name, values, &error);
	g_assert (error == NULL);
}

/**
 * mimedir_attribute_append_parameter_simple:
 * @attribute: a #MIMEDirAttribute
 * @name: parameter name
 * @value: parameter value
 *
 * Appends a parameter to an attribute. If a parameter with the same
 * name does already exist, the value is appended to it.
 **/
void
mimedir_attribute_append_parameter_simple (MIMEDirAttribute *attribute, const gchar *name, const gchar *value)
{
	GError *error = NULL;
	GSList *list = NULL;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (name != NULL && mimedir_utils_is_token (name));
	g_return_if_fail (value != NULL && mimedir_utils_is_qsafe (value));

	list = g_slist_append (list, (gchar *) value);

	mimedir_attribute_internal_append_parameter (attribute, name, list, &error);
	if (error)
		g_critical ("error while appending parameter: %s", error->message);

	g_slist_free (list);
}

/**
 * mimedir_attribute_has_parameter:
 * @attribute: a #MIMEDirAttribute
 * @parameter: parameter name
 *
 * Returns %TRUE if the attribute possesses the given parameter. Otherwise
 * %FALSE is returned.
 *
 * Return value: possession indicator
 **/
gboolean
mimedir_attribute_has_parameter (MIMEDirAttribute *attribute, const gchar *parameter)
{
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (parameter != NULL, FALSE);

	return mimedir_attribute_get_parameter (attribute, parameter) != NULL;
}

/**
 * mimedir_attribute_get_parameter_values:
 * @attribute: a #MIMEDirAttribute
 * @parameter: parameter name
 *
 * Returns the list of string values for the given parameter. If the
 * attribute does not possess the parameter, %NULL is returned. Otherwise
 * the list has at least one element. This list must be freed with
 * mimedir_attribute_free_parameter_values().
 *
 * Return value: the value list or %NULL
 **/
GSList *
mimedir_attribute_get_parameter_values (MIMEDirAttribute *attribute, const gchar *parameter)
{
	MIMEDirParameter *p;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (parameter != NULL, FALSE);

	p = mimedir_attribute_get_parameter (attribute, parameter);
	if (p)
		return p->values;
	else
		return NULL;
}

/**
 * mimedir_attribute_get_parameter_value:
 * @attribute: a #MIMEDirAttribute
 * @parameter: parameter name
 *
 * Returns the value for the given parameter. If the attribute does not
 * possess the parameter, %NULL is returned.
 *
 * Return value: the value or %NULL
 **/
const gchar *
mimedir_attribute_get_parameter_value (MIMEDirAttribute *attribute, const gchar *parameter)
{
	MIMEDirParameter *p;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (parameter != NULL, FALSE);

	p = mimedir_attribute_get_parameter (attribute, parameter);
	if (p && p->values)
		return (gchar *) p->values->data;
	else
		return NULL;
}

/**
 * mimedir_attribute_free_parameter_values:
 * @attribute: a #MIMEDirAttribute
 * @list: the value list
 *
 * Frees an parameter list as obtained through
 * mimedir_attribute_get_parameter_values().
 **/
void
mimedir_attribute_free_parameter_values (MIMEDirAttribute *attribute, GSList *list)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	/* no op */
}

/**
 * mimedir_attribute_set_value:
 * @attribute: a #MIMEDirAttribute
 * @value: the value to set
 *
 * Sets the attribute value literally to the given value; no interpretation
 * or manipulation of the value is done.
 **/
void
mimedir_attribute_set_value (MIMEDirAttribute *attribute, const gchar *value)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (value != NULL);

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = g_string_new (value);

	// FIXME: encode
}

/**
 * mimedir_attribute_get_value:
 * @attribute: a #MIMEDirAttribute
 *
 * Retrieves the attribute value literally from the attribute object.
 * This is guaranteed to be an UTF-8 string.
 *
 * Return value: the attribute value
 **/
const gchar *
mimedir_attribute_get_value (MIMEDirAttribute *attribute)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);

	g_assert (attribute->priv->value != NULL);

	return attribute->priv->value->str;
}

/**
 * mimedir_attribute_set_value_data:
 * @attribute: a #MIMEDirAttribute
 * @value: the value to set
 * @len: length of data
 *
 * Sets the attribute value to the given value.
 **/
void
mimedir_attribute_set_value_data (MIMEDirAttribute *attribute, const gchar *value, gssize len)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (value != NULL);

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = g_string_new_len (value, len);

	// FIXME: encode with base64
}

/**
 * mimedir_attribute_get_value_data:
 * @attribute: a #MIMEDirAttribute
 * @len: pointer to a storage for the data length or %NULL
 *
 * Retrieves the attribute value literally from the attribute object as raw
 * data. Free the returned value with g_free().
 *
 * Return value: the attribute value
 **/
gchar *
mimedir_attribute_get_value_data (MIMEDirAttribute *attribute, gssize *len)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);

	g_assert (attribute->priv->value != NULL);

	if (len)
		*len = attribute->priv->value->len;

	return g_memdup (attribute->priv->value->str, attribute->priv->value->len);
}

void
mimedir_attribute_set_value_uri (MIMEDirAttribute *attribute, const gchar *uri)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (uri != NULL);

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = g_string_new (uri);

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_URI;
}


gchar *
mimedir_attribute_get_value_uri (MIMEDirAttribute *attribute, GError **error)
{
	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	return g_strdup (attribute->priv->value->str);
}


/**
 * mimedir_attribute_set_value_text:
 * @attribute: a #MIMEDirAttribute
 * @text: the text to set
 *
 * Sets the value of the attribute to a single text value.
 **/
void
mimedir_attribute_set_value_text (MIMEDirAttribute *attribute, const gchar *text)
{
	GError *err = NULL;
	gchar *value = NULL;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (text != NULL);

	value = escape (text, &err);

	if (err) {
		g_critical ("value contains invalid characters");
		return;
	}

	mimedir_attribute_set_value_text_unescaped (attribute, value);

	g_free (value);
}

/**
 * mimedir_attribute_set_value_text_unescaped:
 * @attribute: a #MIMEDirAttribute
 * @text: the text to set
 *
 * Sets the value of the attribute to a single text value without escaping
 * characters like ",",".", etc.
 **/
void
mimedir_attribute_set_value_text_unescaped (MIMEDirAttribute *attribute, const gchar *text)
{
	GError *err = NULL;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (text != NULL);

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = g_string_new (text);

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_TEXT;
}
/**
 * mimedir_attribute_get_value_text:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a single text value
 * and returns it. The returned string should be freed with g_free().
 * If the attribute value can't be interpreted as text, @error will be
 * set and %NULL will be returned.
 *
 * Return value: the text value or %NULL
 **/
gchar *
mimedir_attribute_get_value_text (MIMEDirAttribute *attribute, GError **error)
{
	const gchar *data;
	gchar *s;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	data = mimedir_attribute_get_value (attribute);
	if (!data)
		return NULL;

	s = unescape (data, error);

	return s;
}


static gboolean
append_value_text (MIMEDirAttribute *attribute, GString *string, const gchar *text, gboolean escape_semicolon)
{
	gunichar uc;

	if (!text)
		return TRUE;

	uc = g_utf8_get_char(text);
	while (uc != '\0') {
		switch (uc) {
		case ',':
			g_string_append (string, "\\,");
			break;
		case ';':
			if (escape_semicolon)
				g_string_append (string, "\\;");
			else
				g_string_append (string, ";");
			break;
		case '\\':
			g_string_append (string, "\\\\");
			break;
		case '\n':
			g_string_append (string, "\\n");
			break;
		default:
			if (uc != '\t' && (! g_unichar_isprint(uc)))
				return FALSE;
			g_string_append_unichar (string, uc);
			break;
		}

		text = g_utf8_next_char(text);
		uc = g_utf8_get_char(text);
	}

	return TRUE;
}

/**
 * mimedir_attribute_set_value_text_list:
 * @attribute: a #MIMEDirAttribute
 * @list: list of string pointers
 *
 * Sets the attribute to a list of comma-separated text values.
 **/
void
mimedir_attribute_set_value_text_list (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		if (!append_value_text (attribute, string, (const gchar *) list->data, FALSE)) {
			g_critical ("text value contains invalid characters");
			return;
		}

		if (list->next)
			g_string_append_c (string, ',');
	}

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_TEXT;
}

/**
 * mimedir_attribute_get_value_text_list:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a comma-separated list of
 * text values and returns the list. The returned list should be freed
 * with mimedir_attribute_free_text_list(). If the attribute value can't
 * be interpreted as text list, @error will be set and %NULL will be
 * returned.
 *
 * Return value: the text list or %NULL
 **/
GSList *
mimedir_attribute_get_value_text_list (MIMEDirAttribute *attribute, GError **error)
{
	GSList *list = NULL;
	GString *string;
	const gchar *data, *text;
	gunichar uc;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	data = mimedir_attribute_get_value (attribute);
	if (!data)
		return NULL;
	text = data;

	string = g_string_new ("");

	uc = g_utf8_get_char(text);
	while (uc != '\0') {
		switch (uc) {
		case '\\':
			text = g_utf8_next_char(text);
			uc = g_utf8_get_char(text);
			switch (uc) {
			case '\\':
				g_string_append_c (string, '\\');
				break;
			case ';':
			case ',':
				g_string_append_c (string, uc);
				break;
			case 'n':
			case 'N':
				g_string_append_c (string, '\n');
				break;
			default:
				if (uc != '\t' && (! g_unichar_isprint(uc))) {
					g_string_free (string, TRUE);
					g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR, text[0], "text");
					return NULL;
				}
				g_string_append_unichar (string, uc);
				break;
			}
			break;
		case ',':
			list = g_slist_append (list, g_string_free (string, FALSE));
			string = g_string_new ("");
			break;
		case '\r':
			/* ignore */
			break;
		case '\n':
			g_string_append_unichar (string, uc);
			break;
		default:
			if (uc != '\t' && (! g_unichar_isprint(uc))) {
				g_string_free (string, TRUE);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR, MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR, text[0], "text");
				return NULL;
			}
			g_string_append_unichar (string, uc);
			break;
		}

		text = g_utf8_next_char(text);
		uc = g_utf8_get_char(text);
	}

	list = g_slist_append (list, g_string_free (string, FALSE));

	return list;
}


static void
mimedir_attribute_append_tz_to_string (GString *string, gint16 timezone)
{
	switch (timezone) {
	case MIMEDIR_DATETIME_NOTZ:
		/* do nothing */
		break;
	case MIMEDIR_DATETIME_UTC:
		g_string_append_c (string, 'Z');
		break;
	default:
		if (timezone < -1500 || timezone > 1500)
			g_critical ("Invalid time zone specification");

		g_string_append_c (string, timezone >= 0 ? '+' : '-');
		g_string_append_printf (string, "%04d", timezone);

		break;
	}
}

/**
 * mimedir_attribute_set_value_datetime:
 * @attribute: a #MIMEDirAttribute
 * @datetime: a #MIMEDirDateTime
 *
 * Sets the attribute's value to the supplied date and time.
 **/
void
mimedir_attribute_set_value_datetime (MIMEDirAttribute *attribute, MIMEDirDateTime *datetime)
{
	GSList *l = NULL;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (datetime != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (datetime));

	l = g_slist_append (l, datetime);
	mimedir_attribute_set_value_datetime_list (attribute, l);
	g_slist_free (l);
}

/**
 * mimedir_attribute_get_value_datetime:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to interpret the attribute's value as a single date/time value.
 * It honors the VALUE parameter if it is set. A newly allocated
 * #MIMEDirDateTime object will be returned. It is set to the date
 * and time read. If an error occurs, @error will be set and %NULL
 * will be returned. Otherwise the returned object must be freed with
 * g_object_unref().
 *
 * Return value: a newly allocated #MIMEDirDateTime object
 **/
MIMEDirDateTime *
mimedir_attribute_get_value_datetime (MIMEDirAttribute *attribute, GError **error)
{
	GError *err = NULL;
	MIMEDirDateTime *dt;
	GSList *list;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	list = mimedir_attribute_get_value_datetime_list (attribute, &err);
	if (err) {
		g_propagate_error (error, err);
		return NULL;
	}

	if (g_slist_length (list) != 1) {
		mimedir_attribute_free_datetime_list (list);
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, mimedir_attribute_get_name (attribute));
		return NULL;
	}

	g_assert (MIMEDIR_IS_DATETIME (list->data));
	dt = MIMEDIR_DATETIME (list->data);
	g_object_ref (G_OBJECT (dt));

	mimedir_attribute_free_datetime_list (list);

	return dt;
}

/**
 * mimedir_attribute_set_value_datetime_list:
 * @attribute: an #MIMEDirAttribute
 * @list: a list of #MIMEDirDateTime pointers
 *
 * Sets the attribute's value to a comma-separated list of times and dates.
 **/
void
mimedir_attribute_set_value_datetime_list (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;
	MIMEDirDateTime *dt;
	MIMEDirDateTimeFlags flags;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	if (!list->data) {
		g_critical ("MIMEDirDateTime list must contain at least one element.\n");
		return;
	}

	g_return_if_fail (list->data != NULL);
	g_return_if_fail (MIMEDIR_IS_DATETIME (list->data));
	dt = MIMEDIR_DATETIME (list->data);

	flags = dt->flags;
	if ((dt->flags & (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME)) == 0) {
		g_critical ("MIMEDirDateTime object %p has neither date nor time.\n", dt);
		return;
	}

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		g_return_if_fail (list->data != NULL);
		g_return_if_fail (MIMEDIR_IS_DATETIME (list->data));
		dt = MIMEDIR_DATETIME (list->data);

		if (dt->flags != flags) {
			g_critical ("All MIMEDirDateTime object must have the same date/time configuration.\n");
			return;
		}

		if (string->str[0] != '\0')
			g_string_append_c (string, ',');

		if ((flags & (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME)) ==
		    (MIMEDIR_DATETIME_DATE | MIMEDIR_DATETIME_TIME)) {
			g_string_append_printf (string, "%04d%02d%02dT%02d%02d%02d",
						dt->year, dt->month,  dt->day,
						dt->hour, dt->minute, dt->second);
			mimedir_attribute_append_tz_to_string (string, dt->timezone);
		} else if (flags & MIMEDIR_DATETIME_DATE) {
			g_string_append_printf (string, "%02d%02d%02d",
						dt->year, dt->month,  dt->day);
		} else {
			g_string_append_printf (string, "%02d%02d%02d",
						dt->hour, dt->minute, dt->second);
			mimedir_attribute_append_tz_to_string (string, dt->timezone);
		}
	}

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;
	attribute->priv->type  = MIMEDIR_ATTRIBUTE_TYPE_DATETIME;
}

/**
 * mimedir_attribute_get_value_datetime_list:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a comma-separated list of
 * date and time values and returns the list. The returned list should be
 * freed with mimedir_attribute_free_datetime_list(). If the attribute
 * value can't be interpreted as date/time list, @error will be set and
 * %NULL will be returned.
 *
 * Return value: a list of #MIMEDirDateTime pointers
 **/
GSList *
mimedir_attribute_get_value_datetime_list (MIMEDirAttribute *attribute, GError **error)
{
	GSList *list = NULL;
	const gchar *text;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	text = attribute->priv->value->str;
	
	for (;;) {
		MIMEDirDateTime *dt;
		gsize pos;
		gchar *s;

		/* Skip leading white space */

		for (; g_ascii_isspace (text[0]); text++)
			;

		/* Find date end */

		for (pos = 0; !g_ascii_isspace (text[pos]) && text[pos] != ',' && text[pos] != '\0'; pos++)
			;

		s = g_strndup (text, pos);
		text += pos;

		/* Parse date */

		dt = mimedir_datetime_new_parse (s);
		g_free (s);

		if (!dt) {
			mimedir_utils_free_object_slist (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "date/time", attribute->priv->name);
			return NULL;
		}

		list = g_slist_append (list, dt);

		/* Skip trailing white space */

		for (; g_ascii_isspace (text[0]); text++)
			;

		if (text[0] == '\0')
			break;
		else if (text[0] != ',') {
			mimedir_utils_free_object_slist (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE, MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR, attribute->priv->name);
			return NULL;
		}
	}

	return list;
}

/**
 * mimedir_attribute_free_datetime_list:
 * @list: a #GSList
 *
 * Frees a list of #MIMEDirAttributeDateTime pointers as returned by
 * mimedir_attribute_get_value_datetime_list().
 **/
void
mimedir_attribute_free_datetime_list (GSList *list)
{
	GSList *item;

	for (item = list; item != NULL; item = g_slist_next (item)) {
		g_return_if_fail (MIMEDIR_IS_DATETIME (list->data));

		g_object_unref (G_OBJECT (list->data));
	}

	g_slist_free (list);
}

/**
 * mimedir_attribute_set_value_bool:
 * @attribute: a #MIMEDirAttribute
 * @b: boolean value
 *
 * Sets the attribute's value to a boolean value.
 **/
void
mimedir_attribute_set_value_bool (MIMEDirAttribute *attribute, gboolean b)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = g_string_new (b ? "TRUE" : "FALSE");

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_BOOLEAN;
}

/**
 * mimedir_attribute_get_value_bool:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a boolean value and returns.
 * it. If the attribute value can't be interpreted as boolean value, @error
 * will be set and %NULL will be returned.
 *
 * Return value: a boolean value
 **/
gboolean
mimedir_attribute_get_value_bool (MIMEDirAttribute *attribute, GError **error)
{
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_assert (attribute->priv->value != NULL);

	if (!g_ascii_strcasecmp (attribute->priv->value->str, "TRUE"))
		return TRUE;
	else if (!g_ascii_strcasecmp (attribute->priv->value->str, "FALSE"))
		return FALSE;

	g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "boolean", attribute->priv->name);

	return FALSE;
}

/**
 * mimedir_attribute_set_value_int:
 * @attribute: an #MIMEDirAttribute
 * @i: a #gint value
 *
 * Sets the attribute's value to this integer value.
 **/
void
mimedir_attribute_set_value_int (MIMEDirAttribute *attribute, gint i)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	string = g_string_new ("");
	g_string_append_printf (string, "%d", i);

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_INTEGER;
}

/**
 * mimedir_attribute_get_value_int:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a single integer value
 * and returns it. If the attribute value can't be interpreted as an
 * integer value, @error will be set.
 *
 * Return value: a #gint value
 **/
gint
mimedir_attribute_get_value_int (MIMEDirAttribute *attribute, GError **error)
{
	GError *err = NULL;
	GSList *list = NULL;
	gint i;

	g_return_val_if_fail (attribute != NULL, 0);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), 0);
	g_return_val_if_fail (error == NULL || *error == NULL, 0);

	g_assert (attribute->priv->value != NULL);

	list = mimedir_attribute_get_value_int_list (attribute, &err);
	if (err) {
		g_propagate_error (error, err);
		return 0;
	}

	if (g_slist_length (list) != 1) {
		g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "int", attribute->priv->name);
		return 0;
	}

	i = GPOINTER_TO_INT (list->data);

	mimedir_attribute_free_int_list (list);

	return i;
}

/**
 * mimedir_attribute_set_value_int_list:
 * @attribute: an #MIMEDirAttribute
 * @list: a list of #gint values
 *
 * Sets the attribute's value to a comma-separated list of integer values.
 **/
void
mimedir_attribute_set_value_int_list (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (list != NULL);

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		gint i = GPOINTER_TO_INT (list->data);

		if (string->str[0] != '\0')
			g_string_append_c (string, ',');
		g_string_append_printf (string, "%d", i);
	}

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_INTEGER;
}

/**
 * mimedir_attribute_get_value_int_list:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a comma-separated list of
 * integer values and returns the list. The returned list should be
 * freed with mimedir_attribute_free_int_list(). If the attribute value
 * can't be interpreted as int list, @error will be set and %NULL will be
 * returned.
 *
 * Return value: a list of #gint values
 **/
GSList *
mimedir_attribute_get_value_int_list (MIMEDirAttribute *attribute, GError **error)
{
	GSList *list = NULL;
	const gchar *text;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	text = attribute->priv->value->str;

	for (;;) {
		gboolean positive = TRUE;
		gint number = 0;

		/* Sign */

		switch (text[0]) {
		case '-':
			positive = FALSE;
			/* fall through */
		case '+':
			text++;
			break;
		}

		/* Read Number */

		if (text[0] < '0' || text[0] > '9') {
			mimedir_attribute_free_int_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "int", attribute->priv->name);
			return NULL;
		}

		for (; text[0] >= '0' && text[0] <= '9'; text++) {
			number *= 10;
			number += text[0] - '0';
		}

		if (!positive)
			number = -number;

		/* Next Item */

		list = g_slist_append (list, GINT_TO_POINTER (number));

		if (text[0] == '\0')
			break;
		else if (text[0] != ',') {
			mimedir_attribute_free_int_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "int", attribute->priv->name);
			return NULL;
		}

		text++;
	}

	return list;
}

/**
 * mimedir_attribute_set_value_float_list:
 * @attribute: an #MIMEDirAttribute
 * @list: a list of #gdouble pointers
 *
 * Sets the attribute's value to a comma-separated list of floating point
 * values.
 **/
void
mimedir_attribute_set_value_float_list (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));
	g_return_if_fail (list != NULL);

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		gdouble d = *((gdouble *) list->data);

		if (string->str[0] != '\0')
			g_string_append_c (string, ',');
		g_string_append_printf (string, "%.6f", d);
	}

	g_string_free (string, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_FLOAT;
}

/**
 * mimedir_attribute_get_value_float_list:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a comma-separated list of
 * floating point values and returns the list. The returned list should be
 * freed with mimedir_attribute_free_float_list(). If the attribute value
 * can't be interpreted as a list of floating point values, @error will be
 * set and %NULL will be returned.
 *
 * Return value: a list of #gdouble pointers
 **/
GSList *
mimedir_attribute_get_value_float_list (MIMEDirAttribute *attribute, GError **error)
{
	GSList *list = NULL;
	const gchar *text;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_assert (attribute->priv->value != NULL);

	text = attribute->priv->value->str;

	for (;;) {
		gboolean positive = TRUE;
		gdouble number = 0.0;
		gdouble *mem;

		/* Sign */

		switch (text[0]) {
		case '-':
			positive = FALSE;
			/* fall through */
		case '+':
			text++;
			break;
		}

		/* Read Number */

		if (text[0] < '0' || text[0] > '9') {
			mimedir_attribute_free_int_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "int", attribute->priv->name);
			return NULL;
		}

		for (; text[0] >= '0' && text[0] <= '9'; text++) {
			number *= 10;
			number += text[0] - '0';
		}

		if (text[0] == '.') {
			gdouble frac = 0.0;

			text++;

			if (text[0] < '0' || text[0] > '9') {
				mimedir_attribute_free_int_list (list);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "int", attribute->priv->name);
				return NULL;
			}

			for (; text[0] >= '0' && text[0] <= '9'; text++) {
				frac /= 10.0;
				frac += text[0] - '0' / 10.0;
			}

			number += frac;
		}

		/* Next Item */

		mem = g_memdup (&number, sizeof (gdouble));
		list = g_slist_append (list, mem);

		if (text[0] == '\0')
			break;
		else if (text[0] != ',') {
			mimedir_attribute_free_int_list (list);
			g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT, MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR, "float", attribute->priv->name);
			return NULL;
		}

		text++;
	}

	return list;
}

/**
 * mimedir_attribute_set_value_structured_text:
 * @attribute: an #MIMEDirAttribute
 * @list: a list of #GSLists, containing string pointers
 *
 * Sets the attribute's value to a structured text list, i.e. a
 * semicolon-separated list of comma-separated text strings.
 **/
void
mimedir_attribute_set_value_structured_text (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
		GSList *list2;

		for (list2 = (GSList *) list->data; list2 != NULL; list2 = g_slist_next (list2)) {
			if (!append_value_text (attribute, string, (const gchar *) list2->data, TRUE)) {
				g_critical ("text value contains invalid characters");
				return;
			}
			if (list2->next)
				g_string_append_c (string, ',');
		}

		if (list->next)
			g_string_append_c (string, ';');
	}

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_STRUCTURED_TEXT;
}

/**
 * mimedir_attribute_get_value_structured_text:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a structured text type,
 * i.e. where a comma-separated list of strings is in turn separated by
 * semi-colons. The returned list should be freed with
 * mimedir_attribute_free_structured_text_list(). If the attribute value
 * can't be interpreted as a structured text type, @error will be set and
 * %NULL will be returned.
 *
 * Return value: a list of #GSLists, containing string pointers
 **/
GSList *
mimedir_attribute_get_value_structured_text (MIMEDirAttribute *attribute, GError **error)
{
	GSList *list = NULL, *list2 = NULL;
	GString *string;
	const gchar *text;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	text = attribute->priv->value->str;

	if (!g_utf8_validate(text, -1, NULL)) {
	  g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("invalid UTF-8"), attribute->priv->value->str);
	  return NULL;
	}

	string = g_string_new ("");

	for (;;) {
		gunichar uc = g_utf8_get_char(text);
		switch (uc) {
		case '\0':
			list2 = g_slist_append (list2, g_string_free (string, FALSE));
			list  = g_slist_append (list, list2);
			return list;
		case ';':
			list2 = g_slist_append (list2, g_string_free (string, FALSE));
			string = g_string_new ("");
			list  = g_slist_append (list, list2);
			list2 = NULL;
			break;
		case ',':
			list2 = g_slist_append (list2, g_string_free (string, FALSE));
			string = g_string_new ("");
			break;
		case '\\':
			text = g_utf8_next_char(text);
			switch (g_utf8_get_char(text)) {
			case '\\':
				g_string_append_c (string, '\\');
				break;
			case 'n':
			case 'N':
				g_string_append_c (string, '\n');
				break;
			case ';':
			case ',':
				g_string_append_c (string, text[0]);
				break;
			default:
				g_string_free (string, TRUE);
				list = g_slist_append (list, list2);
				mimedir_attribute_free_structured_text_list (list);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("illegal character"), attribute->priv->value->str);
				return NULL;
			}
			break;
		case '\r':
			/* ignore */
			break;
		case '\n':
			g_string_append_unichar (string, uc);
			break;
		default:
			if (text[0] != '\t' && (!g_unichar_isprint(uc))) {
				g_string_free (string, TRUE);
				list = g_slist_append (list, list2);
				mimedir_attribute_free_structured_text_list (list);
				g_set_error (error, MIMEDIR_ATTRIBUTE_ERROR, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX, MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR, _("illegal character"), attribute->priv->value->str);
				return NULL;
			}
			g_string_append_unichar (string, uc);
			break;
		}
		text = g_utf8_next_char(text);;
	}
}

/**
 * mimedir_attribute_set_value_parameters:
 * @attribute: an #MIMEDirAttribute
 * @list: a list of null-terminated strings
 *
 * Sets the attribute's value to a list of key/value pairs. @list
 * must contain an even number of strings, where the first string
 * of each string pair is the key, while each other string is the
 * corresponding value.
 **/
void
mimedir_attribute_set_value_parameters (MIMEDirAttribute *attribute, GSList *list)
{
	GString *string;

	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	string = g_string_new ("");

	for (; list != NULL; list = g_slist_next (list)) {
	        if (!append_value_text (attribute, string, (const gchar *) list->data, TRUE)) {
		        g_critical ("text value contains invalid characters");
			return;
		}
		list = g_slist_next (list);
		if (!list) {
		        g_critical ("list parameter contains an odd number of elements");
			return;
		}
		g_string_append_c (string, '=');
	        if (!append_value_text (attribute, string, (const gchar *) list->data, TRUE)) {
		        g_critical ("text value contains invalid characters");
			return;
		}
		if (list->next)
		        g_string_append_c (string, ';');
	}

	g_string_free (attribute->priv->value, TRUE);
	attribute->priv->value = string;

	attribute->priv->type = MIMEDIR_ATTRIBUTE_TYPE_PARAMETERS;
}

/**
 * mimedir_attribute_get_value_parameters:
 * @attribute: a #MIMEDirAttribute
 * @error: error storage location or %NULL
 *
 * Tries to parse the value of an attribute as a list of parameters.
 * Such a list is a semicolon-separated list of key/value pairs, where
 * key and value are separated by an equality sign (=). The returned list
 * is guaranteed to contain an even number of string pointers, where each pair
 * of strings is a key/value pair. The list should be freed with
 * mimedir_attribute_free_string_list(). If the attribute value can't be
 * interpreted as a parameter type, @error will be set and %NULL will be
 * returned.
 *
 * Return value: a list of string pointers
 **/
GSList *
mimedir_attribute_get_value_parameters (MIMEDirAttribute *attribute, GError **error)
{
	const gchar *text;
	gsize pos = 0;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	text = attribute->priv->value->str;

	return parse_parameter_list(text, &pos, '\0', FALSE, error);
}

/**
 * mimedir_attribute_set_attribute_type:
 * @attribute: a #MIMEDirAttribute
 * @type: the new type
 *
 * Sets the attribute's type.
 */
void
mimedir_attribute_set_attribute_type (MIMEDirAttribute *attribute, MIMEDirAttributeType type)
{
	g_return_if_fail (attribute != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute));

	attribute->priv->type = type;
}

/**
 * mimedir_attribute_get_attribute_type:
 * @attribute: a #MIMEDirAttribute
 *
 * Gets the attribute's type.
 *
 * Return value: the attribute type
 */
MIMEDirAttributeType
mimedir_attribute_get_attribute_type (MIMEDirAttribute *attribute)
{
	g_return_val_if_fail (attribute != NULL, MIMEDIR_ATTRIBUTE_TYPE_UNKNOWN);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), MIMEDIR_ATTRIBUTE_TYPE_UNKNOWN);

	return attribute->priv->type;
}


void
mimedir_attribute_free_list (GSList *list)
{
	GSList *item;

	for (item = list; item != NULL; item = g_slist_next (item))
		g_free (item->data);

	g_slist_free (list);
}

/**
 * mimedir_attribute_free_int_list:
 * @list: a #GSList
 *
 * Frees a list of integer values as returned by
 * mimedir_attribute_get_value_int_list().
 **/
void
mimedir_attribute_free_int_list (GSList *list)
{
	g_slist_free (list);
}

/**
 * mimedir_attribute_free_float_list:
 * @list: a #GSList
 *
 * Frees a list of #gdouble pointers as returned by
 * mimedir_attribute_get_value_float_list().
 **/
void
mimedir_attribute_free_float_list (GSList *list)
{
	GSList *item;

	for (item = list; item != NULL; item = g_slist_next (item))
		g_free (item->data);

	g_slist_free (list);
}

/**
 * mimedir_attribute_free_string_list:
 * @list: a #GSList
 *
 * Frees a list of strings.
 **/
void
mimedir_attribute_free_string_list (GSList *list)
{
        GSList *current;

	for (current = list; current != NULL; current = g_slist_next (current))
	        g_free (current->data);

	g_slist_free (list);
}

/**
 * mimedir_attribute_free_structured_text_list:
 * @list: a #GSList
 *
 * Frees a list as returned by mimedir_attribute_get_value_structured_text().
 **/
void
mimedir_attribute_free_structured_text_list (GSList *list)
{
	GSList *current;

	for (current = list; current != NULL; current = g_slist_next (current)) {
		GSList *current2;

		for (current2 = (GSList *) current->data; current2 != NULL; current2 = g_slist_next (current2))
			g_free (current2->data);
		g_slist_free ((GSList *) current->data);
	}
	g_slist_free (list);
}

/**
 * mimedir_attribute_parse:
 * @attribute: a #MIMEDirAttribute
 * @string: string to parse
 * @error: error storage location or %NULL
 *
 * Clears the attribute object and re-initializes it with data read from the
 * string. If an error occurs during the read, @error will be set and
 * -1 will be returned. Otherwise the number of consumed characters is
 * returned.
 *
 * Return value: number of consumed characters or -1
 **/
ssize_t
mimedir_attribute_parse (MIMEDirAttribute *attribute, const gchar *string, GError **error)
{
	ssize_t len;
	gboolean success;

	GString *gstring;

	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (string != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	gstring = g_string_new (NULL);
 
	len = get_attribute_string (string, gstring);
	success = parse_attribute (attribute, gstring->str, error);

	g_string_free (gstring, TRUE);

	return success ? len : -1;
}

/**
 * mimedir_attribute_write_to_channel:
 * @attribute: a #MIMEDirAttribute
 * @channel: an I/O channel
 * @error: error storage location or %NULL
 *
 * Writes the attribute to the supplied channel. If an error occurs during
 * the write, @error will be set and %FALSE will be returned. Otherwise
 * %TRUE is returned.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_attribute_write_to_channel (MIMEDirAttribute *attribute, GIOChannel *channel, GError **error)
{
	MIMEDirAttributePriv *priv;
	GSList *l;

	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = attribute->priv;

	/* Group/name */

	if (priv->group) {
		if (g_io_channel_write_chars (channel, priv->group, -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, ".", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
	}
	if (g_io_channel_write_chars (channel, priv->name, -1, NULL, error) != G_IO_STATUS_NORMAL)
		return FALSE;

	/* Parameters */

	for (l = priv->parameters; l != NULL; l = g_slist_next (l)) {
		MIMEDirParameter *para;
		GSList *l2;

		if (g_io_channel_write_chars (channel, ";", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		para = (MIMEDirParameter *) l->data;
		if (g_io_channel_write_chars (channel, para->name, -1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		if (g_io_channel_write_chars (channel, "=", 1, NULL, error) != G_IO_STATUS_NORMAL)
			return FALSE;
		for (l2 = para->values; l2 != NULL; l2 = g_slist_next (l2)) {
			const gchar *v = (const gchar *) l2->data;

			if (mimedir_utils_is_safe (v)) {
				if (g_io_channel_write_chars (channel, v, -1, NULL, error) != G_IO_STATUS_NORMAL)
					return FALSE;
			} else {
				if (g_io_channel_write_chars (channel, "\"", 1, NULL, error) != G_IO_STATUS_NORMAL)
					return FALSE;
				if (g_io_channel_write_chars (channel, v, -1, NULL, error) != G_IO_STATUS_NORMAL)
					return FALSE;
				if (g_io_channel_write_chars (channel, "\"", 1, NULL, error) != G_IO_STATUS_NORMAL)
					return FALSE;
			}

			if (l2->next) {
				if (g_io_channel_write_chars (channel, ",", 1, NULL, error) != G_IO_STATUS_NORMAL)
					return FALSE;
			}
		}
	}

	/* Content */

	if (g_io_channel_write_chars (channel, ":", 1, NULL, error) != G_IO_STATUS_NORMAL)
		return FALSE;
	if (g_io_channel_write_chars (channel, priv->value->str, priv->value->len, NULL, error) != G_IO_STATUS_NORMAL)
		return FALSE;
	if (g_io_channel_write_chars (channel, "\n", 1, NULL, error) != G_IO_STATUS_NORMAL)
		return FALSE;

	return TRUE;
}
