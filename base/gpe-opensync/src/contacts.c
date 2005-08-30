#include "gpe_sync.h"

/*! \brief Add a contact
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the contact
 * \param data		Must be a vcard
 */
osync_bool gpe_contacts_add_item(OSyncContext *ctx, const char *data, gchar **error)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("GPE_SYNC", 2, "Adding item:\n%s", data);
	gchar *result = NULL;
	osync_bool state = FALSE;

	gpesync_client_exec_printf (env->client, "add vcard %s", client_callback_string, &result, error, data);

	if (!strcasecmp (result, "OK\n"))
        	state = TRUE;
	
	if (result)
		g_free (result);
	
	return state;
}


/*! \brief Modifies a contact
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the contact
 * \param data		Must be a vcard
 */
osync_bool gpe_contacts_modify_item(OSyncContext *ctx, unsigned int urn, const char *data, gchar **error)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("GPE_SYNC", 2, "Modifying item %d:\n%s", urn, data);
	gchar *result = NULL;
	osync_bool state = FALSE;
	
	gpesync_client_exec_printf (env->client, "modify vcard %d %s", client_callback_string, &result, error, urn, data);
	if (!strcasecmp (result, "OK\n"))
        	state = TRUE;
	
	if (result)
		g_free (result);
	
	return state;
}

/*! \brief Deletes an item with a given urn
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the contact
 */
osync_bool gpe_contacts_delete_item(OSyncContext *ctx, unsigned int urn, gchar **error)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	osync_debug("GPE_SYNC", 2, "Deleting item: %d", urn);
	gchar *result = NULL;
	osync_bool state = FALSE;
	gpesync_client_exec_printf (env->client, "del vcard %d", client_callback_string, &result, error, urn);
	fprintf (stderr, "Delete resut: %s\n", result);

	if (!strcasecmp (result, "OK\n"))
        	state = TRUE;
	
	if (result)
		g_free (result);
	
	return state;
}



/*! \brief Provides access to the contacts
 *
 * \param ctx		Context of the plugin
 * \param change	The change
 */
static osync_bool gpe_contacts_access(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	int urn;
	gchar *error = NULL;
	GString *hash = g_string_new ("");

	switch(osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			if (delete_item (ctx, "contact", urn, &error) == FALSE)
			{
				osync_debug ("GPE_SYNC", 0, "Unable to remove contact %d", urn);
				osync_context_report_error (ctx, OSYNC_ERROR_FILE_NOT_FOUND, error);
				g_free (error);
				return FALSE;
			}

			break;
		case CHANGE_ADDED:
			if (add_item(ctx, "contact", osync_change_get_data (change), &error) == FALSE)
			{
				osync_debug ("GPE_SYNC", 0, "Couldn't add contact");
				osync_context_report_error (ctx, OSYNC_ERROR_EXISTS, error);
				g_free (error);
				return FALSE;
			}

			g_string_printf (hash, "%d", (int) time(NULL));
			osync_change_set_hash (change, hash->str);
			break;
		case CHANGE_MODIFIED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			if (modify_item (ctx, "contact", urn, osync_change_get_data (change), &error) == FALSE)
			{
				osync_debug ("GPE_SYNC", 0, "Unable to modify contact");
				osync_context_report_error (ctx, OSYNC_ERROR_GENERIC, error);
				g_free (error);
				return FALSE;
			}
			g_string_printf (hash, "%d", (int) time(NULL));
			osync_change_set_hash (change, hash->str);
			break;
		default:
			osync_debug("GPE_SYNC", 0, "Unknown change type");
	}
	g_string_free (hash, TRUE);

	osync_context_report_success(ctx);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
	return TRUE;		
}

/*! \brief Commits a change into the GPE-Database
 *
 * \param ctx		The context of the plugin
 * \param change	The item that has changed
 */
static osync_bool gpe_contacts_commit_change (OSyncContext *ctx, OSyncChange *change)
{
	osync_debug ("GPE-SYNC", 4, "start: %s", __func__);
	osync_debug ("GPE-SYNC", 3, "Writing change %s with changetype %i", osync_change_get_uid(change), osync_change_get_changetype(change));

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	if (!gpe_contacts_access(ctx, change))
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
void gpe_contacts_get_changes(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	if (osync_member_get_slow_sync(env->member, "contact"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "contact");
	}
	gchar *errmsg = NULL;
	GSList *uid_list = NULL, *iter;
	gpesync_client_exec (env->client, "uidlist vcard", client_callback_list, &uid_list, &errmsg);

	if (errmsg)
	{
	  /* TODO: Report osync_error */
	  fprintf (stderr, "Error while getting uidlist:%s\n", errmsg);
	}

	GString *vcard_data = g_string_new("");

	for (iter = uid_list; iter; iter = iter->next)
	{
		/* The list we got has the format:
		 * uid:modified */
	  	gchar *modified = NULL;
		gchar *uid = NULL;

		if (parse_uid_modified ((gchar *)iter->data, &uid, &modified) == FALSE)
		{
			fprintf (stderr, "Wrong list item :%s\n", (gchar *)iter->data);
			return;
		}

		g_string_assign (vcard_data, "");
		gpesync_client_exec_printf (env->client, "get vcard %s", client_callback_gstring, &vcard_data, &errmsg, uid);
		osync_debug("GPE_SYNC", 2, "vcard output:\n%s", vcard_data->str);

		report_change (ctx, "contact", uid, modified, vcard_data->str);
		
		g_string_assign (vcard_data, "");
		g_free (iter->data);
	}
	g_string_free (vcard_data, TRUE);

	osync_hashtable_report_deleted(env->hashtable, ctx, "contact");
	g_slist_free (uid_list);
	
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
}

/*! \brief Tells the plugin to accept contacts
 *
 * \param info	The plugin info on which to operat
 */
void gpe_contacts_setup(OSyncPluginInfo *info)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", gpe_contacts_commit_change);
	osync_plugin_set_access_objformat(info, "contact", "vcard30", gpe_contacts_access);
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

}
