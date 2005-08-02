#include "gpe_sync.h"

/*! \brief Connects to the contacts database
 *
 * \param env		Environment of the plugin
 * \param error		Will be fillen when an error occurs
 */
osync_bool gpe_contacts_connect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	gchar *path;

	path = g_strdup_printf ("%s@%s:.gpe/%s", env->username, env->device_addr, "contacts");

	printf("Connecting to contacts: %s\n", path);

        char *nsqlc_err;
        env->contacts = nsqlc_open_ssh (path, 0, &nsqlc_err);
        if (env->contacts == NULL) {
                osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, nsqlc_err);
		env->contacts = NULL;
                return FALSE;
	}
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return TRUE;
}

/*! \brief Closes the connection to the database
 *
 * \param ctx	Context of the plugin
 */
void gpe_contacts_disconnect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	if (env->contacts) {
		nsqlc_close(env->contacts);
	}
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

	return;
}

/*! \brief Provides access to the contacts
 *
 * \param ctx		Context of the plugin
 * \param change	The change
 */
static osync_bool gpe_contacts_commit_change(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);

	switch(osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			osync_debug("GPE_SYNC", 0, "Deleting item");
			printf("Deleting uid: %s\n", osync_change_get_uid(change));
			break;
		case CHANGE_ADDED:
			osync_debug("GPE_SYNC", 0, "Adding item");
			printf("Adding uid: %s\n", osync_change_get_uid(change));
		case CHANGE_MODIFIED:
			osync_debug("GPE_SYNC", 0, "Modify item");
			printf("Modifying uid: %s\n", osync_change_get_uid(change));
			break;
		default:
			osync_debug("GPE_SYNC", 0, "Unknown change type");
	}
	
	return TRUE;		
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);
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
	printf ("Getting changes: contacts\n");
	if (env->contacts == NULL) {
		printf ("Error: No connection!\n");
	}
	
	if (osync_member_get_slow_sync(env->member, "contact"))
	{
		osync_debug("GPE_SYNC", 3, "Slow sync requested");
		osync_hashtable_set_slow_sync(env->hashtable, "contact");
	}

	GSList *uid_list, *i;
	uid_list = fetch_uid_list (env->contacts, "select distinct urn from contacts");

	for (i = uid_list; i; i = i->next)
	{
		GSList *tags;
		OSyncChange *change = osync_change_new();
		MIMEDirVCard *vcard;
		gchar *string;
		gchar *hash;
		GString *uid;
		int urn = (int)i->data;

		tags = fetch_tag_data (env->contacts, "select tag,value from contacts where urn=%d", urn);

		vcard = vcard_from_tags (tags);
	
		printf ("converting to vcard ...\n");
		string = mimedir_vcard_write_to_string (vcard);
		printf ("... done\n");
		g_object_unref (vcard);

		uid = g_string_new("0");
		g_string_printf(uid, "%d", urn);
		osync_change_set_uid(change, uid->str);
		osync_change_set_objformat_string(change, "vcard30");
		hash=get_tag_value(tags, "MODIFIED");
		// If we weren't able to get the value of the MODIFIED-tag
		// then set the hash to 0.
		if(!hash) {
			hash=(gchar*)g_malloc0(sizeof(gchar)*2);
			hash[0]='0';
		}
		
		osync_change_set_hash(change, hash);
		osync_change_set_data(change, (char*)string, strlen(string), TRUE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
		}
		
		printf ("contact: %shash: %s\n", string,hash);
		g_string_free(uid, FALSE);
		g_free(hash);
		gpe_tag_list_free (tags);
	}
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
	osync_debug("GPE_SYNC", 4, "stop %s", __func__);

}
