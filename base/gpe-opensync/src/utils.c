/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
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
 * 
 */

#include "gpe_sync.h"

/*! \brief Reads the uid from "gpe-contact-0123"
 *
 * \param string	The string with the type and uid
 *
 * \return the part behind the last '-'
 */
int get_type_uid (const char *string)
{
  if (!string)
    return 0;
  gchar *p;
  p = strrchr (string, '-');
  *p++;
  return atoi(p);
}

/*! \brief Seperates a string of the format "<a>:<b>" into "<a>" and "<b>"
 *
 * \param string	The string that should be separated
 * \param value		The string where <a> should be stored
 * \param modified	The string where <b> should be stored
 *
 * Note: The pointers of value and modified are pointing into string, so
 * if string gets freed, value and modified are no more valid!
 *
 */
osync_bool parse_value_modified (gchar *string, gchar **value, gchar **modified)
{
	gchar *c = NULL;
	*value = string;
	*modified = (strchr (string, ':'));
	if (*modified == NULL)
		return FALSE;
	*modified += 1;
	
	c = strchr (*modified, '\n');
	if (c)
		c[0] = '\0';
	
	c = strchr (*value, ':');
	c[0] = '\0';

	return TRUE;
}

/*! \brief Reports a single change (vcard, vtodo, vevent)
 *
 * \param ctx	The context of the plugin
 * \param type	Must be "contact", "event", or "todo"
 * \param uid	The uid of the item
 * \param hash	The hash of the item
 * \param data	The whole vcard, vevent or vtodo
 *
 */
osync_bool report_change (OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, gchar *data)
{
	osync_debug("GPE_SYNC", 3, "reporting item type: %s uid: %s hash: %s data size: %d", type, uid, hash, strlen (data));

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data (ctx);
	OSyncChange *change;
	change = osync_change_new ();

	/* We report the uids as
	 * gpe-contacts-0123, gpe-todo-0123, etc. so that file-sync
	 * seperates the different types.
	 */
	gchar buf[25];
	sprintf (buf, "gpe-%s-%s", type, uid);
	osync_change_set_uid (change, g_strdup(buf));

	if (!strcmp (type, "contact"))
	{
		osync_change_set_objtype_string (change, "contact");
		osync_change_set_objformat_string (change, "vcard30");
	}
	else if (!strcmp (type, "event"))
	{
		osync_change_set_objtype_string (change, "event");
		osync_change_set_objformat_string (change, "vevent20");
	}
	else if (!strcmp (type, "todo"))
	{
		osync_change_set_objtype_string (change, "todo");
		osync_change_set_objformat_string (change, "vtodo20");
	}

	osync_change_set_hash (change, g_strdup(hash));
	osync_change_set_data (change, g_strdup(data), strlen (data), TRUE);
	
	if (osync_hashtable_detect_change (env->hashtable, change)) {
		osync_context_report_change (ctx, change);
		osync_hashtable_update_hash (env->hashtable, change);
	}

	return TRUE;
}
