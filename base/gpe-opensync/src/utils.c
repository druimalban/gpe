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


osync_bool report_change (OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, gchar *data)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data (ctx);
	OSyncChange *change = osync_change_new ();
	osync_change_set_uid (change, g_strdup(uid));

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
