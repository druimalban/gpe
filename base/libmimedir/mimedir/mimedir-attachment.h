/* RFC 2445 iCal Attachment Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attachment.h 109 2002-12-11 03:09:14Z srittau $
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

#ifndef __MIMEDIR_ATTACHMENT_H__
#define __MIMEDIR_ATTACHMENT_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-attribute.h"


#define MIMEDIR_TYPE_ATTACHMENT			(mimedir_attachment_get_type())
#define MIMEDIR_ATTACHMENT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_ATTACHMENT, MIMEDirAttachment))
#define MIMEDIR_ATTACHMENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_ATTACHMENT, MIMEDirAttachmentClass))
#define MIMEDIR_IS_ATTACHMENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_ATTACHMENT))
#define MIMEDIR_IS_ATTACHMENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_ATTACHMENT))

typedef struct _MIMEDirAttachment	MIMEDirAttachment;
typedef struct _MIMEDirAttachmentClass	MIMEDirAttachmentClass;
typedef struct _MIMEDirAttachmentPriv	MIMEDirAttachmentPriv;

struct _MIMEDirAttachment
{
	GObject parent;

	MIMEDirAttachmentPriv *priv;
};

struct _MIMEDirAttachmentClass
{
	GObjectClass parent_class;
};


GType			 mimedir_attachment_get_type		(void);

MIMEDirAttachment	*mimedir_attachment_new			(void);
MIMEDirAttachment	*mimedir_attachment_new_from_attribute	(MIMEDirAttribute *attribute, GError **error);

gboolean		 mimedir_attachment_read_from_attribute	(MIMEDirAttachment *attachment, MIMEDirAttribute *attribute, GError **error);

MIMEDirAttribute	*mimedir_attachment_write_to_attribute	(MIMEDirAttachment *attachment);

#endif /* __MIMEDIR_ATTACHMENT_H__ */
