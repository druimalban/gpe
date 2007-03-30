/* RFC 2445 iCal Period Object
 * Copyright (C) 2002  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-period.h 147 2003-07-08 14:49:37Z srittau $
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

#ifndef __MIMEDIR_PERIOD_H__
#define __MIMEDIR_PERIOD_H__

#include <glib.h>
#include <glib-object.h>


#define MIMEDIR_TYPE_PERIOD		(mimedir_period_get_type())
#define MIMEDIR_PERIOD(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_PERIOD, MIMEDirPeriod))
#define MIMEDIR_PERIOD_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_PERIOD, MIMEDirPeriodClass))
#define MIMEDIR_IS_PERIOD(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_PERIOD))
#define MIMEDIR_IS_PERIOD_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_PERIOD))

typedef struct _MIMEDirPeriod		MIMEDirPeriod;
typedef struct _MIMEDirPeriodClass	MIMEDirPeriodClass;
typedef struct _MIMEDirPeriodPriv	MIMEDirPeriodPriv;

struct _MIMEDirPeriod
{
	GObject parent;

	MIMEDirPeriodPriv *priv;
};

struct _MIMEDirPeriodClass
{
	GObjectClass parent_class;
};


GType			 mimedir_period_get_type		(void);

MIMEDirPeriod		*mimedir_period_new			(void);
MIMEDirPeriod		*mimedir_period_new_parse		(const gchar *string);

gchar			*mimedir_period_get_as_mimedir		(MIMEDirPeriod *period);

#endif /* __MIMEDIR_PERIOD_H__ */
