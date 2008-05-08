/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#ifndef _IMPORT_VCARD_H
#define _IMPORT_VCARD_H

GQuark gpecontact_impexport_error_quark (void);
extern int import_vcard (const gchar *filename, GError **error);

#define GPECONTACT_IMPEXPORT_ERROR gpecontact_impexport_error_quark ()

typedef enum {
  GPECONTACT_IMPEXPORT_ERROR_COMPER,
  GPECONTACT_IMPEXPORT_ERROR_DBOPEN
} GpecontactImpexportError;

#define GPECONTACT_IMPEXPORT_ERROR_COMPER_STR _("cannot save contact to database")
#define GPECONTACT_IMPEXPORT_ERROR_DBOPEN_STR _("cannot open contacts database")

#endif /* _IMPORT-VCARD_H */
