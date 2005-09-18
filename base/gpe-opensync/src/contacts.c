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

/*! \brief Commits changes to the gpe client
 *
 * \param ctx		The current context of the sync
 * \param change	The change that has to be committed
 *
 */
osync_bool gpe_contacts_commit_change (OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	gchar *result = NULL;
	gchar *modified = NULL;
	gchar *error = NULL;
	osync_bool state = FALSE;
		
	switch (osync_change_get_changetype (change)) {
		case CHANGE_DELETED:
			osync_debug("GPE_SYNC", 3, "contacts: delete item %d", get_type_uid (osync_change_get_uid (change)));
			gpesync_client_exec_printf (env->client, "del vcard %d", client_callback_string, &result, &error, get_type_uid (osync_change_get_uid (change)));
			break;
		case CHANGE_ADDED:
			osync_debug("GPE_SYNC", 3, "contacts: adding item: %s", osync_change_get_data (change));
			gpesync_client_exec_printf (env->client, "add vcard %s", client_callback_string, &result, &error, osync_change_get_data (change));
			break;
		case CHANGE_MODIFIED:
			osync_debug("GPE_SYNC", 3, "contacts: modifying item %d: %s", get_type_uid (osync_change_get_uid (change)), osync_change_get_data (change));
			gpesync_client_exec_printf (env->client, "modify vcard %d %s", client_callback_string, &result, &error, get_type_uid (osync_change_get_uid (change)), osync_change_get_data (change));
			break;
		default:
			osync_debug ("GPE_SYNC", 0, "Unknown change type");
	}

	osync_debug("GPE_SYNC", 3, "commit result: %s", result);

	if (parse_value_modified (result, &result, &modified))
	{
		if (!strcasecmp (result, "OK"))
		{
			state = TRUE;
			if (osync_change_get_changetype (change) == CHANGE_ADDED)
			{
				/* We need to return the uid of the item
				 * in the gpe databases also, so that the
				 * mapping in opensync is correct.
				 * Fortunately the command "add" returns
				 * on success:
				 * OK:MODIFIED:UID
				 * so we can split value again */
				gchar *uid = NULL;
				gchar buf[25];
				parse_value_modified (modified, &modified, &uid);
				sprintf (buf, "gpe-contact-%s", uid);
				osync_change_set_uid (change, g_strdup (buf));
			}
			osync_change_set_hash (change, modified);
			osync_hashtable_update_hash (env->hashtable, change);
			osync_context_report_success(ctx);
		}
		else {
			osync_debug ("GPE_SYNC", 0, "Couldn't commit contact: %s", error);
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't commit contact: %s", error);
			g_free (error);
		}
	} else {
		osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't process answer form gpesyncd: %s", result);
	}
	
	if (result)
		g_free (result);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
	
	return state;
}

/*! \brief Reports all available items to opensync
 *
 * \param ctx		Context of the plugin
 *
 */
osync_bool gpe_contacts_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_bool state = FALSE;
	
	if (osync_member_get_slow_sync(env->member, "contact"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "contact");
	}

	gchar *errmsg = NULL;
	GSList *uid_list = NULL, *iter;
	gpesync_client_exec (env->client, "uidlist vcard", client_callback_list, &uid_list, &errmsg);

	if ((uid_list) && (!strncasecmp ((gchar *)uid_list->data, "ERROR", 5)))
	{
	  errmsg = (gchar *) uid_list->data;
	}
	
	if (errmsg)
	{
		if (strcasecmp (errmsg, "Error: No item found\n"))
		{
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Error getting contact uidlist: %s\n", errmsg);
		} else {
		  	/* We haven't found any items, so go on. */
			osync_debug("GPE_SYNC", 3, "Found no items");
			uid_list = NULL;
			state = TRUE;
		}
	
		g_slist_free (uid_list);
		uid_list = NULL;

		g_free (errmsg);
	}
	else
	  state = TRUE;

	GString *vcard_data = g_string_new("");

	for (iter = uid_list; iter; iter = iter->next)
	{
		/* The list we got has the format:
		 * uid:modified */
	  	gchar *modified = NULL;
		gchar *uid = NULL;

		if (parse_value_modified ((gchar *)iter->data, &uid, &modified) == FALSE)
		{
			osync_context_report_error (ctx, OSYNC_ERROR_CONVERT, "Wrong uidlist item: %s\n");
			g_slist_free (uid_list);

			return FALSE;
		}

		g_string_assign (vcard_data, "");
		gpesync_client_exec_printf (env->client, "get vcard %s", client_callback_gstring, &vcard_data, &errmsg, uid);
		osync_debug("GPE_SYNC", 2, "vcard output:\n%s", vcard_data->str);

		report_change (ctx, "contact", uid, modified, vcard_data->str);
		
		g_free (iter->data);

		/* We don't need to free modified and uid, because they
		 * are only pointers to iter->data */
		modified = NULL;
		uid = NULL;
	}
	g_string_free (vcard_data, TRUE);

	osync_hashtable_report_deleted(env->hashtable, ctx, "contact");
	if (uid_list)
		g_slist_free (uid_list);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return state;
}


/*! \brief Tells the plugin to accept contacts
 *
 * \param info	The plugin info on which to operate
 */
void gpe_contacts_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", gpe_contacts_commit_change);
	osync_plugin_set_access_objformat(info, "contact", "vcard30", gpe_contacts_commit_change);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}
