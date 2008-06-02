/* RFC 2425 MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-profile.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_PROFILE_H__
#define __MIMEDIR_PROFILE_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_PROFILE_ERROR mimedir_profile_error_quark ()

typedef enum {
	MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE,
	MIMEDIR_PROFILE_ERROR_UNEXPECTED_END,
	MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING,
	MIMEDIR_PROFILE_ERROR_UNMATCHED_END,
	MIMEDIR_PROFILE_ERROR_WRONG_PROFILE
} MIMEDirProfileError;

#define MIMEDIR_PROFILE_ERROR_DUPLICATE_ATTRIBUTE_STR	_("attribute %s defined twice; first occurrence: %s")
#define MIMEDIR_PROFILE_ERROR_UNEXPECTED_END_STR	_("unexpected end of profile")
#define MIMEDIR_PROFILE_ERROR_ATTRIBUTE_MISSING_STR	_("required attribute %s is missing")
#define MIMEDIR_PROFILE_ERROR_UNMATCHED_END_STR		_("unmatched END attribute")
#define MIMEDIR_PROFILE_ERROR_WRONG_PROFILE_STR		_("wrong profile %s; expected %s")


#define MIMEDIR_TYPE_PROFILE		(mimedir_profile_get_type())
#define MIMEDIR_PROFILE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_PROFILE, MIMEDirProfile))
#define MIMEDIR_PROFILE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_PROFILE, MIMEDirProfileClass))
#define MIMEDIR_IS_PROFILE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_PROFILE))
#define MIMEDIR_IS_PROFILE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_PROFILE))

typedef struct _MIMEDirProfile		MIMEDirProfile;
typedef struct _MIMEDirProfileClass	MIMEDirProfileClass;
typedef struct _MIMEDirProfilePriv	MIMEDirProfilePriv;

struct _MIMEDirProfile
{
	GObject parent;

	MIMEDirProfilePriv *priv;
};

struct _MIMEDirProfileClass
{
	GObjectClass parent_class;
};


GQuark			 mimedir_profile_error_quark		(void);
GType			 mimedir_profile_get_type		(void);

MIMEDirProfile		*mimedir_profile_new			(const gchar *type);

void			 mimedir_profile_set_charset		(MIMEDirProfile *profile, const gchar *charset);
const gchar		*mimedir_profile_get_charset		(MIMEDirProfile *attribute);

ssize_t			 mimedir_profile_parse			(MIMEDirProfile *profile, const gchar *string, GError **error);
gboolean		 mimedir_profile_write_to_channel	(MIMEDirProfile *profile, GIOChannel *channel, GError **error);
gchar			*mimedir_profile_write_to_string	(MIMEDirProfile *profile);

void			 mimedir_profile_append_attribute	(MIMEDirProfile *profile, MIMEDirAttribute *attribute);
MIMEDirAttribute	*mimedir_profile_get_attribute		(MIMEDirProfile *profile, const gchar *name);
GSList			*mimedir_profile_get_attribute_list	(MIMEDirProfile *profile);

void			 mimedir_profile_append_subprofile	(MIMEDirProfile *profile, MIMEDirProfile *sub_profile);
GSList			*mimedir_profile_get_subprofiles	(MIMEDirProfile *profile);

#endif /* __MIMEDIR_PROFILE_H__ */
