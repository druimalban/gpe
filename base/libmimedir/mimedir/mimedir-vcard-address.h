/* RFC 2426 vCard MIME Directory Profile -- Address Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard-address.h 72 2002-07-31 12:51:31Z srittau $
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

#ifndef __MIMEDIR_VCARD_ADDRESS_H__
#define __MIMEDIR_VCARD_ADDRESS_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_TYPE_VCARD_ADDRESS		(mimedir_vcard_address_get_type())
#define MIMEDIR_VCARD_ADDRESS(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCARD_ADDRESS, MIMEDirVCardAddress))
#define MIMEDIR_VCARD_ADDRESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCARD_ADDRESS, MIMEDirVCardClassAddress))
#define MIMEDIR_IS_VCARD_ADDRESS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCARD_ADDRESS))
#define MIMEDIR_IS_VCARD_ADDRESS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCARD_ADDRESS))

typedef struct _MIMEDirVCardAddress		MIMEDirVCardAddress;
typedef struct _MIMEDirVCardAddressClass	MIMEDirVCardAddressClass;
typedef struct _MIMEDirVCardAddressPriv		MIMEDirVCardAddressPriv;


struct _MIMEDirVCardAddress
{
	GObject parent;

	MIMEDirVCardAddressPriv *priv;
};

struct _MIMEDirVCardAddressClass
{
	GObjectClass parent_class;

	void (* changed) (MIMEDirVCardAddress *address);
};


GType			 mimedir_vcard_address_get_type			(void);
MIMEDirVCardAddress	*mimedir_vcard_address_new			(void);
MIMEDirVCardAddress	*mimedir_vcard_address_new_from_attribute	(MIMEDirAttribute *attribute, GError **error);

gboolean		 mimedir_vcard_address_set_from_attribute	(MIMEDirVCardAddress *address, MIMEDirAttribute *attribute, GError **error);
MIMEDirAttribute	*mimedir_vcard_address_save_to_attribute	(MIMEDirVCardAddress *address);

gchar			*mimedir_vcard_address_get_as_string		(MIMEDirVCardAddress *address);
gchar			*mimedir_vcard_address_get_type_string		(MIMEDirVCardAddress *address);
gchar			*mimedir_vcard_address_get_title		(MIMEDirVCardAddress *address);

#endif /* __MIMEDIR_VCARD_ADDRESS_H__ */
