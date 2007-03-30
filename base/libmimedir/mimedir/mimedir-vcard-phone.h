/* RFC 2426 vCard MIME Directory Profile -- Phone Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-phone.h 32 2002-07-21 13:36:40Z srittau $
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

#ifndef __MIMEDIR_VCARD_PHONE_H__
#define __MIMEDIR_VCARD_PHONE_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_TYPE_VCARD_PHONE		(mimedir_vcard_phone_get_type())
#define MIMEDIR_VCARD_PHONE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCARD_PHONE, MIMEDirVCardPhone))
#define MIMEDIR_VCARD_PHONE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCARD_PHONE, MIMEDirVCardClassPhone))
#define MIMEDIR_IS_VCARD_PHONE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCARD_PHONE))
#define MIMEDIR_IS_VCARD_PHONE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCARD_PHONE))

typedef struct _MIMEDirVCardPhone	MIMEDirVCardPhone;
typedef struct _MIMEDirVCardPhoneClass	MIMEDirVCardPhoneClass;
typedef struct _MIMEDirVCardPhonePriv	MIMEDirVCardPhonePriv;


struct _MIMEDirVCardPhone
{
	GObject parent;

	MIMEDirVCardPhonePriv *priv;
};

struct _MIMEDirVCardPhoneClass
{
	GObjectClass parent_class;

	void (* changed) (MIMEDirVCardPhone *phone);
};


GType			 mimedir_vcard_phone_get_type		(void);
MIMEDirVCardPhone	*mimedir_vcard_phone_new		(void);
MIMEDirVCardPhone	*mimedir_vcard_phone_new_from_attribute	(MIMEDirAttribute *attribute, GError **error);

gboolean		 mimedir_vcard_phone_set_from_attribute	(MIMEDirVCardPhone *phone, MIMEDirAttribute *attribute, GError **error);
MIMEDirAttribute	*mimedir_vcard_phone_save_to_attribute	(MIMEDirVCardPhone *phone);

gchar			*mimedir_vcard_phone_get_as_string	(MIMEDirVCardPhone *phone);
gchar			*mimedir_vcard_phone_get_type_string	(MIMEDirVCardPhone *phone);

#endif /* __MIMEDIR_VCARD_PHONE_H__ */
