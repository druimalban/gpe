/* RFC 2426 vCard MIME Directory Profile Object
 * Copyright (C) 2002-2005  Sebastian Rittau <srittau@jroger.in-berlin.de>
 *
 * $Id: mimedir-vcard.h 176 2005-02-26 22:46:04Z srittau $
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

#ifndef __MIMEDIR_VCARD_H__
#define __MIMEDIR_VCARD_H__

#include <glib.h>
#include <glib-object.h>

#include "mimedir-profile.h"
#include "mimedir-vcard-address.h"
#include "mimedir-vcard-email.h"
#include "mimedir-vcard-phone.h"


#define MIMEDIR_VCARD_TIMEZONE_MAX	(+23 * 60 + 59)
#define MIMEDIR_VCARD_TIMEZONE_MIN	(-23 * 60 + 59)
#define MIMEDIR_VCARD_LATITUDE_MAX	(+90.0)
#define MIMEDIR_VCARD_LATITUDE_MIN	(-90.0)
#define MIMEDIR_VCARD_LONGITUDE_MAX	(+180.0)
#define MIMEDIR_VCARD_LONGITUDE_MIN	(-180.0)

typedef enum {
	MIMEDIR_VCARD_KEY_UNKNOWN,	/* Key type is not known */
	MIMEDIR_VCARD_KEY_X509,		/* Key is a X.509 key */
	MIMEDIR_VCARD_KEY_PGP		/* Key is a PGP/GnuPG key */
} MIMEDirVCardKeyType;

typedef struct _MIMEDirVCardKey {
	gchar *key;
	MIMEDirVCardKeyType type;
} MIMEDirVCardKey;


#define MIMEDIR_TYPE_VCARD		(mimedir_vcard_get_type())
#define MIMEDIR_VCARD(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MIMEDIR_TYPE_VCARD, MIMEDirVCard))
#define MIMEDIR_VCARD_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MIMEDIR_TYPE_VCARD, MIMEDirVCardClass))
#define MIMEDIR_IS_VCARD(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIMEDIR_TYPE_VCARD))
#define MIMEDIR_IS_VCARD_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIMEDIR_TYPE_VCARD))

typedef struct _MIMEDirVCard		MIMEDirVCard;
typedef struct _MIMEDirVCardClass	MIMEDirVCardClass;
typedef struct _MIMEDirVCardPriv	MIMEDirVCardPriv;

struct _MIMEDirVCard
{
	GObject parent;

	MIMEDirVCardPriv *priv;
};

struct _MIMEDirVCardClass
{
	GObjectClass parent_class;

	void (* changed) (MIMEDirVCard *card);
};


GList		*mimedir_vcard_read_file		(const gchar *filename, GError **error);
GList		*mimedir_vcard_read_channel		(GIOChannel *channel, GError **error);
void		 mimedir_vcard_free_list		(GList *list);
gboolean	 mimedir_vcard_write_list		(const gchar *filename, GList *list, GError **error);
gboolean	 mimedir_vcard_write_list_to_channel	(GIOChannel *channel, GList *list, GError **error);

GType		 mimedir_vcard_get_type			(void);
MIMEDirVCard	*mimedir_vcard_new			(void);
MIMEDirVCard	*mimedir_vcard_new_from_profile		(MIMEDirProfile *profile, GError **error);

gboolean	 mimedir_vcard_read_from_profile	(MIMEDirVCard *vcard, MIMEDirProfile *profile, GError **error);
MIMEDirProfile	*mimedir_vcard_write_to_profile		(MIMEDirVCard *vcard);
gboolean	 mimedir_vcard_write_to_channel		(MIMEDirVCard *vcard, GIOChannel *channel, GError **error);
gchar		*mimedir_vcard_write_to_string		(MIMEDirVCard *vcard);

void		 mimedir_vcard_set_birthday		(MIMEDirVCard *vcard, MIMEDirDateTime *birthday);
MIMEDirDateTime	*mimedir_vcard_get_birthday		(MIMEDirVCard *vcard);

void		 mimedir_vcard_append_address		(MIMEDirVCard *vcard, MIMEDirVCardAddress *address);
void		 mimedir_vcard_remove_address		(MIMEDirVCard *vcard, MIMEDirVCardAddress *address);
void		 mimedir_vcard_append_email		(MIMEDirVCard *vcard, MIMEDirVCardEMail *email);
void		 mimedir_vcard_remove_email		(MIMEDirVCard *vcard, MIMEDirVCardEMail *email);
void		 mimedir_vcard_append_phone		(MIMEDirVCard *vcard, MIMEDirVCardPhone *phone);
void		 mimedir_vcard_remove_phone		(MIMEDirVCard *vcard, MIMEDirVCardPhone *phone);

MIMEDirVCardAddress *
		mimedir_vcard_get_preferred_address	(MIMEDirVCard *vcard);
MIMEDirVCardEMail *
		mimedir_vcard_get_preferred_email	(MIMEDirVCard *vcard);
MIMEDirVCardPhone *
		mimedir_vcard_get_preferred_phone	(MIMEDirVCard *vcard);

void		 mimedir_vcard_set_timezone		(MIMEDirVCard *vcard, gint timezone);
void		 mimedir_vcard_clear_timezone		(MIMEDirVCard *vcard);
gboolean	 mimedir_vcard_get_timezone		(MIMEDirVCard *vcard, gint *timezone);
void		 mimedir_vcard_set_geo_position		(MIMEDirVCard *vcard, gdouble latitude, gdouble longitude);
void		 mimedir_vcard_clear_geo_position	(MIMEDirVCard *vcard);
gboolean	 mimedir_vcard_get_geo_position		(MIMEDirVCard *vcard, gdouble *latitude, gdouble *longitude);

gchar		*mimedir_vcard_get_as_string		(MIMEDirVCard *vcard);

#endif /* __MIMEDIR_VCARD_H__ */
