/* RFC 2445 vJournal MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vjournal.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VJOURNAL_H__
#define __MIMEDIR_VJOURNAL_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


#define MIMEDIR_TYPE_VJOURNAL			(mimedir_vjournal_get_type())
#define MIMEDIR_VJOURNAL(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VJOURNAL, MIMEDirVJournal))
#define MIMEDIR_VJOURNAL_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VJOURNAL, MIMEDirVJournalClass))
#define MIMEDIR_IS_VJOURNAL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VJOURNAL))
#define MIMEDIR_IS_VJOURNAL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VJOURNAL))

typedef struct _MIMEDirVJournal		MIMEDirVJournal;
typedef struct _MIMEDirVJournalClass	MIMEDirVJournalClass;
typedef struct _MIMEDirVJournalPriv	MIMEDirVJournalPriv;

struct _MIMEDirVJournal
{
	MIMEDirVComponent parent;

	MIMEDirVJournalPriv *priv;
};

struct _MIMEDirVJournalClass
{
	MIMEDirVComponentClass parent_class;
};


GType		 mimedir_vjournal_get_type		(void);
MIMEDirVJournal	*mimedir_vjournal_new			(void);
MIMEDirVJournal	*mimedir_vjournal_new_from_profile	(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_vjournal_read_from_profile	(MIMEDirVJournal *vjournal, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vjournal_write_to_profile	(MIMEDirVJournal *vjournal);
gboolean	 mimedir_vjournal_write_to_channel	(MIMEDirVJournal *vjournal, GIOChannel *channel, GError **error);
gchar		*mimedir_vjournal_write_to_string	(MIMEDirVJournal *vjournal);

#endif /* __MIMEDIR_VJOURNAL_H__ */
