/* RFC 2445 iCalendar MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcal.h 185 2005-03-01 03:01:49Z srittau $
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

#ifndef __MIMEDIR_VCAL_H__
#define __MIMEDIR_VCAL_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcomponent.h"


#define MIMEDIR_VCAL_ERROR mimedir_vcal_error_quark ()

typedef enum {
	MIMEDIR_VCAL_ERROR_UNKNOWN_SCALE
} MIMEDirVCalError;

#define MIMEDIR_VCAL_ERROR_UNKNOWN_SCALE_STR	_("unknown calendar scale %s")

#define MIMEDIR_TYPE_VCAL		(mimedir_vcal_get_type())
#define MIMEDIR_VCAL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCAL, MIMEDirVCal))
#define MIMEDIR_VCAL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCAL, MIMEDirVCalClass))
#define MIMEDIR_IS_VCAL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCAL))
#define MIMEDIR_IS_VCAL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCAL))

typedef struct _MIMEDirVCal		MIMEDirVCal;
typedef struct _MIMEDirVCalClass	MIMEDirVCalClass;
typedef struct _MIMEDirVCalPriv		MIMEDirVCalPriv;

struct _MIMEDirVCal
{
	GObject parent;

	MIMEDirVCalPriv *priv;
};

struct _MIMEDirVCalClass
{
	GObjectClass parent_class;

	void (* changed) (MIMEDirVCal *cal);
};


GQuark		 mimedir_vcal_error_quark		(void);

GList		*mimedir_vcal_read_file			(const gchar *filename, GError **error);
GList		*mimedir_vcal_read_channel		(GIOChannel *channel, GError **error);
void		 mimedir_vcal_free_list			(GList *list);

GType		 mimedir_vcal_get_type			(void);
MIMEDirVCal	*mimedir_vcal_new			(void);
MIMEDirVCal	*mimedir_vcal_new_from_profile		(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_vcal_read_from_profile		(MIMEDirVCal *vcal, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vcal_write_to_profile		(MIMEDirVCal *vcal);
gboolean	 mimedir_vcal_write_to_file		(MIMEDirVCal *vcal, const gchar *filename, GError **error);
gboolean	 mimedir_vcal_write_to_channel		(MIMEDirVCal *vcal, GIOChannel *channel, GError **error);
gchar		*mimedir_vcal_write_to_string		(MIMEDirVCal *vcal);

GSList		*mimedir_vcal_get_component_list	(MIMEDirVCal *vcal);
GSList		*mimedir_vcal_get_event_list		(MIMEDirVCal *vcal);
GSList		*mimedir_vcal_get_todo_list		(MIMEDirVCal *vcal);
void		 mimedir_vcal_free_component_list	(GSList *list);
void		 mimedir_vcal_add_component		(MIMEDirVCal *vcal, MIMEDirVComponent *component);
void		 mimedir_vcal_add_component_list	(MIMEDirVCal *vcal, GList *list);

#endif /* __MIMEDIR_VCAL_H__ */
