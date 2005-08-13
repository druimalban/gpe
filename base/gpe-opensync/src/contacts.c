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

/*! \brief Deletes an item with a given urn
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the contact
 */
void gpe_contacts_delete_item(OSyncContext *ctx, unsigned int urn)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	osync_debug("GPE_SYNC", 2, "Deleting item: %d", urn);
	nsqlc_exec_printf (env->contacts, "delete from contacts where urn='%d'", NULL, NULL, NULL, urn);
	nsqlc_exec_printf (env->contacts, "delete from contacts_urn where urn='%d'", NULL, NULL, NULL, urn);

	return;
}

/*! \brief Adds a new contact
 *
 * \param ctx		The context of the plugin
 * \param urn		The urn of the contact
 * \param data		Must be a vcard
 */
osync_bool gpe_contacts_add_item(OSyncContext *ctx, unsigned int urn, const char *data)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	GSList *tags;
	MIMEDirVCard *vcard=NULL;
	GError *error=NULL;
	GString *string;
	
	osync_debug("GPE_SYNC", 2, "Adding item:\n%s", data);
	vcard = mimedir_vcard_new_from_string (data, &error);
	//printf("Error: %s\n", error->message);
	if (vcard == NULL) {
		osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, error->message);
		printf ("Error converting data to vcard\n");

		return FALSE;
	}
	tags = vcard_to_tags (vcard);

	string = g_string_new(NULL);
	s_print_tags(tags, string);
	osync_debug("GPE_SYNC", 2, "Inserting tags:\n%s", string->str);
//			g_free(vcard);

	// Here we put the data into the database contacts
	store_tag_data (env->contacts, "contacts", urn, tags, TRUE);
	// Now all we need to do, is to add some "previewdata"
	// to the database contacts_urn:
	nsqlc_exec_printf (env->contacts, "insert into contacts_urn values ('%d', '%s', '%s', '%s')", \
			NULL, NULL, NULL, urn, \
			get_tag_value (tags, "name"), \
			get_tag_value (tags, "family_name"), \
			get_tag_value (tags, "company"));

//	g_slist_free(tags);
//

	return TRUE;
}

/*! \brief Provides access to the contacts
 *
 * \param ctx		Context of the plugin
 * \param change	The change
 */
static osync_bool gpe_contacts_access(OSyncContext *ctx, OSyncChange *change)
{
	osync_debug("GPE_SYNC", 4, "start %s", __func__);
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	int urn;	

	switch(osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
			gpe_contacts_delete_item(ctx, urn);

			break;
		case CHANGE_ADDED:
			urn = get_new_urn (env->contacts);		
			if (gpe_contacts_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
				return FALSE;
			
			break;
		case CHANGE_MODIFIED:
			urn = atoi(g_strdup (osync_change_get_uid(change)));
		gpe_contacts_delete_item(ctx, urn);
			if (gpe_contacts_add_item(ctx, urn, osync_change_get_data (change)) == FALSE)
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
		GString *string;
		gchar *hash;
		GString *uid;
		int urn = (int)i->data;

		tags = fetch_tag_data (env->contacts, "select tag,value from contacts where urn=%d", urn);
		string = g_string_new("");
		tags_replace_category (tags, env->categories);
		s_print_tags (tags, string);
		osync_debug("GPE_SYNC", 2, "db output:\n%s", string->str);
		
		vcard = vcard_from_tags (tags);
	
		string = g_string_new(mimedir_vcard_write_to_string (vcard));
		osync_debug("GPE_SYNC", 2, "vcard output:\n%s", string->str);

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
		osync_change_set_data(change, string->str, string->len, TRUE);

		if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
		}
	
		g_string_free(uid, TRUE);
		g_string_free(string, TRUE);
		g_free(hash);
		gpe_tag_list_free (tags);
	}

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
