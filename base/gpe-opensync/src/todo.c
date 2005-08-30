#include "gpe_sync.h"

/*! \brief Connects to the todo database
 *
 * \param env		Environment of the plugin
 * \param error		Will be fillen when an error occurs
 */
osync_bool gpe_todo_connect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return TRUE;
}

/*! \brief Closes the connection to the database
 *
 * \param ctx	Context of the plugin
 */
void gpe_todo_disconnect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return;
}

/*! \brief Deletes an item with a given urn
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the todo item 
 */
void gpe_todo_delete_item(OSyncContext *ctx, unsigned int urn)
{
	return;
}

/*! \brief Adds a new todo 
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the todo 
 * \param data		Must be a vcard
 */
osync_bool gpe_todo_add_item(OSyncContext *ctx, unsigned int urn, const char *data)
{
	osync_debug("GPE_SYNC", 2, "Adding item:\n%s", data);

	return TRUE;
}

/*! \brief Provides access to the todo
 *
 * \param ctx		Context of the plugin
 * \param change	The change
 */
static osync_bool gpe_todo_access(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	int urn=0;	

	switch(osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			gpe_todo_delete_item(ctx, urn);

			break;
		case CHANGE_ADDED:
			if (gpe_todo_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
				return FALSE;
			
			break;
		case CHANGE_MODIFIED:
			urn = atoi (osync_change_get_uid(change));
			gpe_todo_delete_item(ctx, urn);
			if (gpe_todo_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
				return FALSE;
			
			break;
		default:
			osync_debug("GPE_SYNC", 0, "Unknown change type");
	}

	osync_context_report_success(ctx);
	return TRUE;		
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}

/*! \brief Commits a change into the GPE-Database
 *
 * \param ctx		The context of the plugin
 * \param change	The item that has changed
 */
static osync_bool gpe_todo_commit_change (OSyncContext *ctx, OSyncChange *change)
{
	osync_debug ("GPE-SYNC", 4, "start: %s", __func__);
	osync_debug ("GPE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	if (!gpe_todo_access(ctx, change))
		return FALSE;

	osync_hashtable_update_hash(env->hashtable, change);
	osync_debug("GPE-SYNC", 4, "end: %s", __func__);

	return TRUE;
}


/*! \brief Lists all the objects that were changed
 *
 * \param ctx		Context of the plugin
 *
 * In the end it only fills the hashtable with the values
 */
void gpe_todo_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	if (osync_member_get_slow_sync(env->member, "todo"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "todo");
	}

	GSList *uid_list = NULL, *i;

	for (i = uid_list; i; i = i->next)
	{
		OSyncChange *change = osync_change_new();
		gchar *hash;
		GString *uid;
		GString *string = NULL;

		uid = g_string_new (NULL);
				
		osync_debug("GPE_SYNC", 2, "vtodo output:\n%s", string->str);

		osync_change_set_uid(change, uid->str);
		osync_change_set_objtype_string(change, "todo");
		osync_change_set_objformat_string(change, "vtodo20");
		// If we weren't able to get the value of the MODIFIED-tag
		// then set the hash to 0.
		if(!hash) {
			hash=(gchar*)g_malloc0(sizeof(gchar)*2);
			hash[0]='0';
		}
		
		osync_change_set_hash(change, hash);
		printf ("vtodo is: %s", string->str);
		osync_change_set_data(change, string->str, string->len, TRUE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
		}

		// TODO: Why do I have to pass a FALSE here? I don't need
		// to do that at contacts.c
//		g_string_free(string, FALSE);
		g_string_free(uid, TRUE);
		g_free(hash);
	}

	osync_hashtable_report_deleted(env->hashtable, ctx, "todo");
	
	g_slist_free (uid_list);
	
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}

/*! \brief Tells the plugin to accept todo
 *
 * \param info	The plugin info on which to operat
 */
void gpe_todo_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
/*	osync_plugin_accept_objtype(info, "todo");
	osync_plugin_accept_objformat(info, "todo", "vtodo20", NULL);
	osync_plugin_set_commit_objformat(info, "todo", "vtodo20", gpe_todo_commit_change);
	osync_plugin_set_access_objformat(info, "todo", "vtodo20", gpe_todo_access);
	*/
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}
