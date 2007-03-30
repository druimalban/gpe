/* RFC 2445 iCal Component Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcomponent.h 147 2003-07-08 14:49:37Z srittau $
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

#ifndef __MIMEDIR_VCOMPONENT_H__
#define __MIMEDIR_VCOMPONENT_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-recurrence.h"


typedef enum {
	MIMEDIR_CLASS_PUBLIC,
	MIMEDIR_CLASS_PRIVATE,
	MIMEDIR_CLASS_CONFIDENTIAL,
	MIMEDIR_CLASS_CUSTOM
} MIMEDirClassification;

typedef enum {
	MIMEDIR_STATUS_UNKNOWN,
	MIMEDIR_STATUS_CANCELLED,
	MIMEDIR_STATUS_TENTATIVE,	/* VEvent */
	MIMEDIR_STATUS_CONFIRMED,
	MIMEDIR_STATUS_NEEDS_ACTION,	/* VTodo */
	MIMEDIR_STATUS_COMPLETED,
	MIMEDIR_STATUS_IN_PROCESS,
	MIMEDIR_STATUS_DRAFT,		/* VJournal */
	MIMEDIR_STATUS_FINAL
} MIMEDirStatus;

#define MIMEDIR_VCOMPONENT_PERCENT_NONE (101)


#define MIMEDIR_TYPE_VCOMPONENT			(mimedir_vcomponent_get_type())
#define MIMEDIR_VCOMPONENT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCOMPONENT, MIMEDirVComponent))
#define MIMEDIR_VCOMPONENT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCOMPONENT, MIMEDirVComponentClass))
#define MIMEDIR_IS_VCOMPONENT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCOMPONENT))
#define MIMEDIR_IS_VCOMPONENT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCOMPONENT))

typedef struct _MIMEDirVComponent	MIMEDirVComponent;
typedef struct _MIMEDirVComponentClass	MIMEDirVComponentClass;
typedef struct _MIMEDirVComponentPriv	MIMEDirVComponentPriv;

struct _MIMEDirVComponent
{
	GObject parent;

	MIMEDirVComponentPriv *priv;
};

struct _MIMEDirVComponentClass
{
	GObjectClass parent_class;

	void (* changed)        (MIMEDirVComponent *component);
	void (* alarms_changed) (MIMEDirVComponent *component);
};


GType		 mimedir_vcomponent_get_type			(void);

gboolean	 mimedir_vcomponent_read_from_profile		(MIMEDirVComponent *vcomponent, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vcomponent_write_to_profile		(MIMEDirVComponent *vcomponent, const gchar *profile_name);

void		 mimedir_vcomponent_set_attachment_list		(MIMEDirVComponent *vcomponent, GList *list);
GList		*mimedir_vcomponent_get_attachment_list		(MIMEDirVComponent *vcomponent);
gchar		*mimedir_vcomponent_get_categories_as_string	(MIMEDirVComponent *vcomponent);
void		 mimedir_vcomponent_set_classification		(MIMEDirVComponent *vcomponent, MIMEDirClassification classification, const gchar *klass);
MIMEDirClassification
		 mimedir_vcomponent_get_classification		(MIMEDirVComponent *vcomponent, const gchar **klass);
void		 mimedir_vcomponent_set_geopos			(MIMEDirVComponent *vcomponent, gdouble latitude, gdouble longitude);
void		 mimedir_vcomponent_get_geopos			(MIMEDirVComponent *vcomponent, gdouble *latitude, gdouble *longitude);
void		 mimedir_vcomponent_set_location		(MIMEDirVComponent *vcomponent, const gchar *location, const gchar *uri);
const gchar	*mimedir_vcomponent_get_location		(MIMEDirVComponent *vcomponent, const gchar **uri);

void		 mimedir_vcomponent_set_opaque			(MIMEDirVComponent *vcomponent, gboolean opaque);
gboolean	 mimedir_vcomponent_get_opaque			(MIMEDirVComponent *vcomponent);
void		 mimedir_vcomponent_set_allday			(MIMEDirVComponent *vcomponent, gboolean allday);
gboolean	 mimedir_vcomponent_get_allday			(MIMEDirVComponent *vcomponent);

void		 mimedir_vcomponent_set_alarm_list		(MIMEDirVComponent *vcomponent, const GList *list);
const GList	*mimedir_vcomponent_get_alarm_list		(MIMEDirVComponent *vcomponent);

void		 mimedir_vcomponent_set_attendee_list		(MIMEDirVComponent *vcomponent, const GList *list);
const GList	*mimedir_vcomponent_get_attendee_list		(MIMEDirVComponent *vcomponent);

gboolean	 mimedir_vcomponent_does_recur			(MIMEDirVComponent *vcomponent);
void		 mimedir_vcomponent_set_recurrence		(MIMEDirVComponent *vcomponent, MIMEDirRecurrence *recurrence);
MIMEDirRecurrence *
		 mimedir_vcomponent_get_recurrence		(MIMEDirVComponent *vcomponent);
MIMEDirDateTime	*mimedir_vcomponent_get_next_occurence		(MIMEDirVComponent *vcomponent, MIMEDirDateTime *after);
GList		*mimedir_vcomponent_get_occurences_between	(MIMEDirVComponent *vcomponent, MIMEDirDateTime *start, MIMEDirDateTime *end);
void		 mimedir_vcomponent_free_occurences		(GList *list);

#endif /* __MIMEDIR_VCOMPONENT_H__ */
