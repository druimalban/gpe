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
osync_bool gpe_todo_commit_change (OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	gchar *result = NULL;
	gchar *modified = NULL;
	gchar *error = NULL;
	osync_bool state = FALSE;
		
	switch (osync_change_get_changetype (change)) {
		case CHANGE_DELETED:
			osync_debug("GPE_SYNC", 3, "delete item %d", atoi (osync_change_get_uid (change)));
			gpesync_client_exec_printf (env->client, "del vtodo %d", client_callback_string, &result, &error, atoi (osync_change_get_uid (change)));
			break;
		case CHANGE_ADDED:
			osync_debug("GPE_SYNC", 3, "adding item: %s", osync_change_get_data (change));
			gpesync_client_exec_printf (env->client, "add vtodo %s", client_callback_string, &result, &error, osync_change_get_data (change));
			break;
		case CHANGE_MODIFIED:
			osync_debug("GPE_SYNC", 3, "modifying item %d: %s", atoi (osync_change_get_uid (change)), osync_change_get_data (change));
			gpesync_client_exec_printf (env->client, "modify vtodo %d %s", client_callback_string, &result, &error, atoi (osync_change_get_uid (change)), osync_change_get_data (change));
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
			osync_change_set_hash (change, modified);
			osync_hashtable_update_hash (env->hashtable, change);
			osync_context_report_success(ctx);
		}
		else {
			osync_debug ("GPE_SYNC", 0, "Couldn't commit todo: %s", error);
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't commit todo: %s", error);
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
osync_bool gpe_todo_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_bool state = FALSE;
	
	if (osync_member_get_slow_sync(env->member, "todo"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "todo");
	}

	gchar *errmsg = NULL;
	GSList *uid_list = NULL, *iter;
	gpesync_client_exec (env->client, "uidlist vtodo", client_callback_list, &uid_list, &errmsg);

	if (uid_list)
		if (!strncasecmp ((gchar *)uid_list->data, "ERROR", 5))
			errmsg = (gchar *) uid_list->data;
	
	if (errmsg)
	{
		if (strcasecmp (errmsg, "Error: No item found"))
		{	osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Error getting todo uidlist: %s\n", errmsg);
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

	GString *vtodo_data = g_string_new("");

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

		g_string_assign (vtodo_data, "");
		gpesync_client_exec_printf (env->client, "get vtodo %s", client_callback_gstring, &vtodo_data, &errmsg, uid);
		osync_debug("GPE_SYNC", 2, "vtodo output:\n%s", vtodo_data->str);

		report_change (ctx, "todo", uid, modified, vtodo_data->str);
		
		g_free (iter->data);

		/* We don't need to free modified and uid, because they
		 * are only pointers to iter->data */
		modified = NULL;
		uid = NULL;
	}
	g_string_free (vtodo_data, TRUE);

	osync_hashtable_report_deleted(env->hashtable, ctx, "todo");
	
	if (uid_list)
		g_slist_free (uid_list);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return state;
}


/*! \brief Tells the plugin to accept todos
 *
 * \param info	The plugin info on which to operate
 */
void gpe_todo_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
	osync_plugin_set_commit_objformat(info, "todo", "vtodo20", gpe_todo_commit_change);
	osync_plugin_set_access_objformat(info, "todo", "vtodo20", gpe_todo_commit_change);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}
