/* RFC 2445 iCal Attachment Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-attachment.c 176 2005-02-26 22:46:04Z srittau $
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

#include "mimedir-attachment.h"


#ifndef _
#define _(x) (dgettext(GETTEXT_PACKAGE, (x)))
#endif


static void	 mimedir_attachment_class_init		(MIMEDirAttachmentClass	*klass);
static void	 mimedir_attachment_init		(MIMEDirAttachment	*attachment);
static void	 mimedir_attachment_dispose		(GObject		*object);


struct _MIMEDirAttachmentPriv {
};

static GObjectClass *parent_class = NULL;

/*
 * Class and Object Management
 */

GType
mimedir_attachment_get_type (void)
{
	static GType mimedir_attachment_type = 0;

	if (!mimedir_attachment_type) {
		static const GTypeInfo mimedir_attachment_info = {
			sizeof (MIMEDirAttachmentClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) mimedir_attachment_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (MIMEDirAttachment),
			1,    /* n_preallocs */
			(GInstanceInitFunc) mimedir_attachment_init,
		};

		mimedir_attachment_type = g_type_register_static (G_TYPE_OBJECT,
								  "MIMEDirAttachment",
								  &mimedir_attachment_info,
								  0);
	}

	return mimedir_attachment_type;
}


static void
mimedir_attachment_class_init (MIMEDirAttachmentClass *klass)
{
	GObjectClass *gobject_class;

	g_return_if_fail (klass != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTACHMENT_CLASS (klass));

	gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = mimedir_attachment_dispose;

	parent_class = g_type_class_peek_parent (klass);
}


static void
mimedir_attachment_init (MIMEDirAttachment *attachment)
{
	MIMEDirAttachmentPriv *priv;

	g_return_if_fail (attachment != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTACHMENT (attachment));

	priv = g_new0 (MIMEDirAttachmentPriv, 1);
	attachment->priv = priv;
}


static void
mimedir_attachment_dispose (GObject *object)
{
	MIMEDirAttachment *attachment;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MIMEDIR_IS_ATTACHMENT (object));

	attachment = MIMEDIR_ATTACHMENT (object);

	g_free (attachment->priv);
	attachment->priv = NULL;

	G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Public Methods
 */

/**
 * mimedir_attachment_new:
 *
 * Creates a new iCal attachment object.
 *
 * Return value: a new attachment object
 **/
MIMEDirAttachment *
mimedir_attachment_new (void)
{
	MIMEDirAttachment *recur;

	recur = g_object_new (MIMEDIR_TYPE_ATTACHMENT, NULL);

	return recur;
}

/**
 * mimedir_attachment_new_from_attribute:
 * @attribute: a #MIMEDirAttribute object
 * @error: error storage location or %NULL
 *
 * Creates a new attachment object and initializes it with values read from
 * the supplied attribute object.
 *
 * Return value: a new #MIMEDirAttachment object or %NULL on error
 **/
MIMEDirAttachment *
mimedir_attachment_new_from_attribute (MIMEDirAttribute *attribute, GError **error)
{
	MIMEDirAttachment *attachment;

	g_return_val_if_fail (attribute != NULL, NULL);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

        attachment = g_object_new (MIMEDIR_TYPE_ATTACHMENT, NULL);

        if (!mimedir_attachment_read_from_attribute (attachment, attribute, error)) {
                g_object_unref (G_OBJECT (attachment));
                return NULL;
        }

        return attachment;
}

/**
 * mimedir_attachment_read_from_attribute:
 * @attachment: a #MIMEDirAttachment object
 * @attribute: a #MIMEDirAttribute object
 * @error: error storage location or %NULL
 *
 * Initializes @attachment with data read from @attribute.
 *
 * Return value: success indicator
 **/
gboolean
mimedir_attachment_read_from_attribute (MIMEDirAttachment *attachment, MIMEDirAttribute *attribute, GError **error)
{
	g_return_val_if_fail (attachment != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTACHMENT (attachment), FALSE);
	g_return_val_if_fail (attribute != NULL, FALSE);
	g_return_val_if_fail (MIMEDIR_IS_ATTRIBUTE (attribute), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* FIXME */

	return TRUE;
}

/**
 * mimedir_attachment_write_to_attribute:
 * @attachment: a #MIMEDirAttachment object
 *
 * Write @attachment to a newly allocated #MIMEDirAttribute object. The
 * returned object should be freed with g_object_unref().
 * 
 * Return value: A newly allocated #MIMEDirAttribute object
 **/
MIMEDirAttribute *
mimedir_attachment_write_to_attribute (MIMEDirAttachment *attachment)
{
	MIMEDirAttribute *attr;

        g_return_val_if_fail (attachment != NULL, NULL);
        g_return_val_if_fail (MIMEDIR_IS_ATTACHMENT (attachment), NULL);

	attr = mimedir_attribute_new_with_name ("ATTACH");

	/* FIXME */

	return attr;
}
