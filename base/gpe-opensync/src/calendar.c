#include "gpe_sync.h"

/*! \brief Commits changes to the gpe client
 *
 * \param ctx		The current context of the sync
 * \param change	The change that has to be committed
 *
 */
osync_bool gpe_calendar_commit_change (OSyncContext *ctx, OSyncChange *change)
{
        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	gchar *result = NULL;
	gchar *modified = NULL;
	gchar *error = NULL;
	osync_bool state = FALSE;
		
	switch (osync_change_get_changetype (change)) {
		case CHANGE_DELETED:
			gpesync_client_exec_printf (env->client, "del vevent %d", client_callback_string, &result, &error, atoi (osync_change_get_uid (change)));
			break;
		case CHANGE_ADDED:
			gpesync_client_exec_printf (env->client, "add vevent %s", client_callback_string, &result, &error, osync_change_get_data (change));
			break;
		case CHANGE_MODIFIED:
			gpesync_client_exec_printf (env->client, "modify vevent %d %s", client_callback_string, &result, &error, atoi (osync_change_get_uid (change)), osync_change_get_data (change));
			break;
		default:
			osync_debug ("GPE_SYNC", 0, "Unknown change type");
	}

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
			osync_debug ("GPE_SYNC", 0, "Couldn't commit event: %s", error);
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't commit event: %s", error);
			g_free (error);
		}
	} else {
		osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Couldn't process answer form gpesyncd: %s", result);

	}
	
	if (result)
		g_free (result);
	
	return state;
}

/*! \brief Reports all available items to opensync
 *
 * \param ctx		Context of the plugin
 *
 */
void gpe_calendar_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	if (osync_member_get_slow_sync(env->member, "event"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "event");
	}
	gchar *errmsg = NULL;
	GSList *uid_list = NULL, *iter;
	gpesync_client_exec (env->client, "uidlist vevent", client_callback_list, &uid_list, &errmsg);


	if ((uid_list) && (!strncasecmp ((gchar *)uid_list->data, "ERROR", 5)))
	{
	  errmsg = (gchar *) uid_list->data;
	}
	
	if (errmsg)
	{
		if (strcasecmp (errmsg, "Error: No item found\n"))
		{
			g_free (uid_list->data);
			g_slist_free (uid_list);
			uid_list = NULL;
			osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, "Error getting todo uidlist: %s\n", errmsg);
		} else {
			g_free (uid_list->data);
			g_slist_free (uid_list);
			uid_list = NULL;
		}
	}

	GString *vevent_data = g_string_new("");

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

			return;
		}

		g_string_assign (vevent_data, "");
		gpesync_client_exec_printf (env->client, "get vevent %s", client_callback_gstring, &vevent_data, &errmsg, uid);
		osync_debug("GPE_SYNC", 2, "vevent output:\n%s", vevent_data->str);

		report_change (ctx, "event", uid, modified, vevent_data->str);
		
		g_string_assign (vevent_data, "");
		g_free (iter->data);

		/* We don't need to free modified and uid, because they
		 * are only pointers to iter->data */
		modified = NULL;
		uid = NULL;
	}
	g_string_free (vevent_data, TRUE);

	osync_hashtable_report_deleted(env->hashtable, ctx, "event");
	g_slist_free (uid_list);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}


/*! \brief Tells the plugin to accept todos
 *
 * \param info	The plugin info on which to operate
 */
void gpe_calendar_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
	osync_plugin_set_commit_objformat(info, "event", "vevent20", gpe_calendar_commit_change);
	osync_plugin_set_access_objformat(info, "event", "vevent20", gpe_calendar_commit_change);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

}
