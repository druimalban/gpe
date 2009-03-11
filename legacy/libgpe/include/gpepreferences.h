/** Preferences handling for GPE
 *
 * Copyright (C) 2003 Luc Pionchon
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */
#ifndef GPE_PREFERENCES_H
#define GPE_PREFERENCES_H

#include <glib.h>
#include <glib-object.h> //gtype.h -> GType, G_TYPE_*

typedef enum{
  GPE_PREFS_OK = 0,
  GPE_PREFS_ERROR,
} GpePrefsResult;

/** Initialises the preferences backend.
 *   
 *  The DB is located in ~/.gpe/prefs_<prog_name>.db
 *
 *  It is the responsability of the application to give the right name,
 *  and avoid any conflict with other applications. A good candidate is
 *  the name of the application itself. It is possibile to use
 *  variations on <prog_name>, to allow DB migration (for example by a
 *  script called by postinst)
 *
 *  Example:
 *    gpe_prefs_init ("gpe-contacts-v1");
 *    => ~/.gpe/prefs_gpe-contacts-v1.db
 *    => gpe-contacts preferences, DB version v1
 */
GpePrefsResult gpe_prefs_init (gchar * prog_name);

/** Terminates the preferences management session.
 *  eventually saves unsaved preferences.
 *  exits the backend.
 */
void gpe_prefs_exit ();

/** Gets the value of a preference associated to the given key.
 *  The preferences backend must be initialised by gpe_prefs_init().
 */
GpePrefsResult gpe_prefs_get (gchar * key, GType type, gpointer pvalue);

/** Sets the value of a preference associated to the given key.
 *  The preferences backend must be initialised by gpe_prefs_init().
 */
GpePrefsResult gpe_prefs_set (gchar * key, GType type, gconstpointer pvalue);


/* NULL terminated list of (key, type, value) => single transaction */
//GpePrefsResult gpe_prefs_get_v(gchar * key, GType type, gpointer pvalue, ...);
//GpePrefsResult gpe_prefs_set_v(gchar * key, GType type, gconstpointer pvalue,  ...);

#endif
