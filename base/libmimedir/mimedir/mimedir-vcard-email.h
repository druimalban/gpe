/* RFC 2426 vCard MIME Directory Profile -- E-Mail Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-email.h 32 2002-07-21 13:36:40Z srittau $
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

#ifndef __MIMEDIR_VCARD_EMAIL_H__
#define __MIMEDIR_VCARD_EMAIL_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


typedef enum {
	MIMEDIR_VCARD_EMAIL_TYPE_UNKNOWN,
	MIMEDIR_VCARD_EMAIL_TYPE_INTERNET,
	MIMEDIR_VCARD_EMAIL_TYPE_X400
} MIMEDirVCardEMailType;


#define MIMEDIR_TYPE_VCARD_EMAIL		(mimedir_vcard_email_get_type())
#define MIMEDIR_VCARD_EMAIL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCARD_EMAIL, MIMEDirVCardEMail))
#define MIMEDIR_VCARD_EMAIL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCARD_EMAIL, MIMEDirVCardClassEMail))
#define MIMEDIR_IS_VCARD_EMAIL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCARD_EMAIL))
#define MIMEDIR_IS_VCARD_EMAIL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCARD_EMAIL))

typedef struct _MIMEDirVCardEMail	MIMEDirVCardEMail;
typedef struct _MIMEDirVCardEMailClass	MIMEDirVCardEMailClass;
typedef struct _MIMEDirVCardEMailPriv	MIMEDirVCardEMailPriv;


struct _MIMEDirVCardEMail
{
	GObject parent;

	MIMEDirVCardEMailPriv *priv;
};

struct _MIMEDirVCardEMailClass
{
	GObjectClass parent_class;

	void (* changed) (MIMEDirVCardEMail *email);
};


GType			 mimedir_vcard_email_get_type		(void);
MIMEDirVCardEMail	*mimedir_vcard_email_new		(void);
MIMEDirVCardEMail	*mimedir_vcard_email_new_from_attribute	(MIMEDirAttribute *attribute, GError **error);

gboolean		 mimedir_vcard_email_set_from_attribute	(MIMEDirVCardEMail *email, MIMEDirAttribute *attribute, GError **error);
MIMEDirAttribute	*mimedir_vcard_email_save_to_attribute	(MIMEDirVCardEMail *email);

gchar			*mimedir_vcard_email_get_as_string	(MIMEDirVCardEMail *email);
gchar			*mimedir_vcard_email_get_type_string	(MIMEDirVCardEMail *email);

#endif /* __MIMEDIR_VCARD_EMAIL_H__ */
