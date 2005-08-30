#include "gpe_sync.h"

/*! \brief Connects to the calendar database
 *
 * \param env		Environment of the plugin
 * \param error		Will be fillen when an error occurs
 */
osync_bool gpe_calendar_connect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return TRUE;
}

/*! \brief Closes the connection to the database
 *
 * \param ctx	Context of the plugin
 */
void gpe_calendar_disconnect(OSyncContext *ctx)
{
	return;
}

/*! \brief Deletes an item with a given urn
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the calendar item 
 */
void gpe_calendar_delete_item(OSyncContext *ctx, unsigned int urn)
{
	return;
}

/*! \brief Adds a new event 
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the event 
 * \param data		Must be a vcard
 */
osync_bool gpe_calendar_add_item(OSyncContext *ctx, unsigned int urn, const char *data)
{
	return TRUE;
}

/*! \brief Provides access to the calendar
 *
 * \param ctx		Context of the plugin
 * \param change	The change
 */
static osync_bool gpe_calendar_access(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	int urn = 0;	

	switch(osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			gpe_calendar_delete_item(ctx, urn);

			break;
		case CHANGE_ADDED:
			if (gpe_calendar_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
				return FALSE;
			
			break;
		case CHANGE_MODIFIED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			gpe_calendar_delete_item(ctx, urn);
			if (gpe_calendar_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
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
static osync_bool gpe_calendar_commit_change (OSyncContext *ctx, OSyncChange *change)
{
	osync_debug ("GPE-SYNC", 4, "start: %s", __func__);
	osync_debug ("GPE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	if (!gpe_calendar_access(ctx, change))
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
void gpe_calendar_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	if (osync_member_get_slow_sync(env->member, "event"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "event");
	}

	GSList *uid_list = NULL, *i;

	// In calendar it's called uid and not urn as in contacts - ???

	for (i = uid_list; i; i = i->next)
	{
		OSyncChange *change = osync_change_new();
		gchar *hash;
		GString *uid;
		GString *string = NULL;

		uid = g_string_new (NULL);
				
		osync_debug("GPE_SYNC", 2, "vevent output:\n%s", string->str);

		osync_change_set_uid(change, uid->str);
		osync_change_set_objtype_string(change, "event");
		osync_change_set_objformat_string(change, "vevent20");
		
		osync_change_set_hash(change, hash);
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

	osync_hashtable_report_deleted(env->hashtable, ctx, "event");
	
	g_slist_free (uid_list);
	
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}

/*! \brief Tells the plugin to accept calendar
 *
 * \param info	The plugin info on which to operat
 */
void gpe_calendar_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
/*	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
	osync_plugin_set_commit_objformat(info, "event", "vevent20", gpe_calendar_commit_change);
	osync_plugin_set_access_objformat(info, "event", "vevent20", gpe_calendar_access);
*/
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}
