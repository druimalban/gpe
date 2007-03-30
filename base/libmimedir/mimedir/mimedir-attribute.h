/* RFC 2425 MIME Directory Attribute Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attribute.h 209 2005-08-31 14:35:02Z srittau $
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

#ifndef __MIMEDIR_ATTRIBUTE_H__
#define __MIMEDIR_ATTRIBUTE_H__

#include <stdlib.h>
#include <time.h>

#include <glib.h>
#include <glib-object.h>

#include "mimedir/mimedir-datetime.h"


#define MIMEDIR_ATTRIBUTE_ERROR mimedir_attribute_error_quark ()

typedef enum {
	MIMEDIR_ATTRIBUTE_ERROR_SYNTAX,
	MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR,
	MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT,
	MIMEDIR_ATTRIBUTE_ERROR_UNKNOWN_ENCODING,
	MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE,
	MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE,
	MIMEDIR_ATTRIBUTE_ERROR_INVALID_BASE64,
	MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP,
	MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT,
	MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG
} MIMEDirAttributeError;

#define MIMEDIR_ATTRIBUTE_ERROR_SYNTAX_STR			_("syntax error (%s)")
#define MIMEDIR_ATTRIBUTE_ERROR_ILLEGAL_CHAR_STR		_("illegal character 0x%02x for type \"%s\"")
#define MIMEDIR_ATTRIBUTE_ERROR_INVALID_FORMAT_STR		_("invalid format for type \"%s\" in attribute %s")
#define MIMEDIR_ATTRIBUTE_ERROR_UNKNOWN_ENCODING_STR		_("attribute %s could not be decoded, since its encoding is unknown")
#define MIMEDIR_ATTRIBUTE_ERROR_INVALID_VALUE_STR		_("invalid value in attribute %s")
#define MIMEDIR_ATTRIBUTE_ERROR_PARAMETER_NOT_UNIQUE_STR	_("parameter \"%s\" must not be used more than once in attribute %s")
#define MIMEDIR_ATTRIBUTE_ERROR_INVALID_BASE64_STR		_("invalid Base64 sequence")
#define MIMEDIR_ATTRIBUTE_ERROR_INVALID_QP_STR			_("invalid quoted-printable sequence")
#define MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_SHORT_STR		_("attribute list of %s is too short")
#define MIMEDIR_ATTRIBUTE_ERROR_LIST_TOO_LONG_STR		_("attribute list of %s is too long")

typedef enum {
	MIMEDIR_ATTRIBUTE_TYPE_UNKNOWN,
	MIMEDIR_ATTRIBUTE_TYPE_URI,
	MIMEDIR_ATTRIBUTE_TYPE_TEXT,
	MIMEDIR_ATTRIBUTE_TYPE_DATE,
	MIMEDIR_ATTRIBUTE_TYPE_TIME,
	MIMEDIR_ATTRIBUTE_TYPE_DATETIME,
	MIMEDIR_ATTRIBUTE_TYPE_INTEGER,
	MIMEDIR_ATTRIBUTE_TYPE_FLOAT,
	MIMEDIR_ATTRIBUTE_TYPE_BOOLEAN,

	/* VCard */

	MIMEDIR_ATTRIBUTE_TYPE_STRUCTURED_TEXT,
	MIMEDIR_ATTRIBUTE_TYPE_PARAMETERS,
} MIMEDirAttributeType;

typedef enum {
	MIMEDIR_ATTRIBUTE_ENCODING_UNKNOWN,
	MIMEDIR_ATTRIBUTE_ENCODING_BASE64,
	MIMEDIR_ATTRIBUTE_ENCODING_QP, /* read-only */
} MIMEDirAttributeEncoding;
#define MIMEDIR_ATTRIBUTE_ENCODING_NONE MIMEDIR_ATTRIBUTE_ENCODING_UNKNOWN


#define MIMEDIR_TYPE_ATTRIBUTE			(mimedir_attribute_get_type())
#define MIMEDIR_ATTRIBUTE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_ATTRIBUTE, MIMEDirAttribute))
#define MIMEDIR_ATTRIBUTE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_ATTRIBUTE, MIMEDirAttributeClass))
#define MIMEDIR_IS_ATTRIBUTE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_ATTRIBUTE))
#define MIMEDIR_IS_ATTRIBUTE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_ATTRIBUTE))

typedef struct _MIMEDirAttribute	MIMEDirAttribute;
typedef struct _MIMEDirAttributeClass	MIMEDirAttributeClass;
typedef struct _MIMEDirAttributePriv	MIMEDirAttributePriv;

struct _MIMEDirAttribute
{
	GObject parent;

	MIMEDirAttributePriv *priv;
};

struct _MIMEDirAttributeClass
{
	GObjectClass parent_class;
};

GQuark		 mimedir_attribute_error_quark			(void);
GType		 mimedir_attribute_get_type			(void);

MIMEDirAttribute *mimedir_attribute_new				(void);
MIMEDirAttribute *mimedir_attribute_new_with_name		(const gchar *name);

const gchar	*mimedir_attribute_get_name			(MIMEDirAttribute *attribute);
void		 mimedir_attribute_set_group			(MIMEDirAttribute *attribute, const gchar *group);
const gchar	*mimedir_attribute_get_group			(MIMEDirAttribute *attribute);
void		 mimedir_attribute_set_charset			(MIMEDirAttribute *attribute, const gchar *charset);
const gchar	*mimedir_attribute_get_charset			(MIMEDirAttribute *attribute);

void		 mimedir_attribute_append_parameter		(MIMEDirAttribute *attribute, const gchar *name, GSList *values);
void		 mimedir_attribute_append_parameter_simple	(MIMEDirAttribute *attribute, const gchar *name, const gchar *value);
gboolean	 mimedir_attribute_has_parameter		(MIMEDirAttribute *attribute, const gchar *parameter);
GSList		*mimedir_attribute_get_parameter_values		(MIMEDirAttribute *attribute, const gchar *parameter);
const gchar	*mimedir_attribute_get_parameter_value		(MIMEDirAttribute *attribute, const gchar *parameter);
void		 mimedir_attribute_free_parameter_values	(MIMEDirAttribute *attribute, GSList *list);

void		 mimedir_attribute_set_value			(MIMEDirAttribute *attribute, const gchar *value);
const gchar	*mimedir_attribute_get_value			(MIMEDirAttribute *attribute);
void		 mimedir_attribute_set_value_data		(MIMEDirAttribute *attribute, const gchar *uri, gssize len);
gchar		*mimedir_attribute_get_value_data		(MIMEDirAttribute *attribute, gssize *len);
void		 mimedir_attribute_set_value_uri		(MIMEDirAttribute *attribute, const gchar *uri);
gchar		*mimedir_attribute_get_value_uri		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_text		(MIMEDirAttribute *attribute, const gchar *text);
void		 mimedir_attribute_set_value_text_unescaped	(MIMEDirAttribute *attribute, const gchar *text);
gchar		*mimedir_attribute_get_value_text		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_text_list		(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_text_list		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_datetime		(MIMEDirAttribute *attribute, MIMEDirDateTime *datetime);
MIMEDirDateTime	*mimedir_attribute_get_value_datetime		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_datetime_list	(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_datetime_list	(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_free_datetime_list		(GSList *list);
void		 mimedir_attribute_set_value_bool		(MIMEDirAttribute *attribute, gboolean b);
gboolean	 mimedir_attribute_get_value_bool		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_int		(MIMEDirAttribute *attribute, gint i);
gint		 mimedir_attribute_get_value_int		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_int_list		(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_int_list		(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_float_list		(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_float_list		(MIMEDirAttribute *attribute, GError **error);

void		 mimedir_attribute_set_value_structured_text	(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_structured_text	(MIMEDirAttribute *attribute, GError **error);
void		 mimedir_attribute_set_value_parameters		(MIMEDirAttribute *attribute, GSList *list);
GSList		*mimedir_attribute_get_value_parameters		(MIMEDirAttribute *attribute, GError **error);

void		 mimedir_attribute_free_list			(GSList *list);
#define		 mimedir_attribute_free_text_list(list)		mimedir_attribute_free_list(list)
void		 mimedir_attribute_free_int_list		(GSList *list);
void		 mimedir_attribute_free_float_list		(GSList *list);
void		 mimedir_attribute_free_string_list		(GSList *list);
void		 mimedir_attribute_free_structured_text_list	(GSList *list);

void		 mimedir_attribute_set_attribute_type		(MIMEDirAttribute *attribute, MIMEDirAttributeType type);
MIMEDirAttributeType
		 mimedir_attribute_get_attribute_type		(MIMEDirAttribute *attribute);

ssize_t		 mimedir_attribute_parse			(MIMEDirAttribute *attribute, const gchar *string, GError **error);
gboolean	 mimedir_attribute_write_to_channel		(MIMEDirAttribute *attribute, GIOChannel *channel, GError **error);

#endif /* __MIMEDIR_ATTRIBUTE_H__ */
