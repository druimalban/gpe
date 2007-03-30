/* RFC 2445 vFreeBusy MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vfreebusy.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VFREEBUSY_H__
#define __MIMEDIR_VFREEBUSY_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


typedef enum {
        MIMEDIR_VFREEBUSY_FREE,
        MIMEDIR_VFREEBUSY_BUSY,
        MIMEDIR_VFREEBUSY_BUSY_UNAVAILABLE,
        MIMEDIR_VFREEBUSY_BUSY_TENTATIVE
} MIMEDirVFreeBusyType;
                                                                                
                                                                                
#define MIMEDIR_TYPE_VFREEBUSY			(mimedir_vfreebusy_get_type())
#define MIMEDIR_VFREEBUSY(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VFREEBUSY, MIMEDirVFreeBusy))
#define MIMEDIR_VFREEBUSY_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VFREEBUSY, MIMEDirVFreeBusyClass))
#define MIMEDIR_IS_VFREEBUSY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VFREEBUSY))
#define MIMEDIR_IS_VFREEBUSY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VFREEBUSY))

typedef struct _MIMEDirVFreeBusy	MIMEDirVFreeBusy;
typedef struct _MIMEDirVFreeBusyClass	MIMEDirVFreeBusyClass;
typedef struct _MIMEDirVFreeBusyPriv	MIMEDirVFreeBusyPriv;

struct _MIMEDirVFreeBusy
{
	MIMEDirVComponent parent;

	MIMEDirVFreeBusyPriv *priv;
};

struct _MIMEDirVFreeBusyClass
{
	MIMEDirVComponentClass parent_class;
};


GType			 mimedir_vfreebusy_get_type		(void);
MIMEDirVFreeBusy	*mimedir_vfreebusy_new			(void);
MIMEDirVFreeBusy	*mimedir_vfreebusy_new_from_profile	(MIMEDirProfile *profile, GError **error);

gboolean		 mimedir_vfreebusy_read_from_profile	(MIMEDirVFreeBusy *vfreebusy, MIMEDirProfile *profile, GError **error);
MIMEDirProfile		*mimedir_vfreebusy_write_to_profile	(MIMEDirVFreeBusy *vfreebusy);
gboolean		 mimedir_vfreebusy_write_to_channel	(MIMEDirVFreeBusy *vfreebusy, GIOChannel *channel, GError **error);
gchar			*mimedir_vfreebusy_write_to_string	(MIMEDirVFreeBusy *vfreebusy);

#endif /* __MIMEDIR_VFREEBUSY_H__ */
