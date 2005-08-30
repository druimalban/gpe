#include "gpe_sync.h"

osync_bool add_item (OSyncContext *ctx, const char *type, const char *data, gchar **error)
{
        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("GPE_SYNC", 2, "Adding item:\n%s", data);
	gchar *result = NULL;
	osync_bool state = FALSE;
	gchar vtype[7] = "";

	if (!strcmp (type, "contact"))
		sprintf (vtype, "vcard");
	else if (!strcmp (type, "event"))
		sprintf (vtype, "vevent");
	else if (!strcmp (type, "todo"))
		sprintf (vtype, "vtodo");
			
	gpesync_client_exec_printf (env->client, "add %s %s", client_callback_string, &result, error, vtype, data);

	if (!strcasecmp (result, "OK\n"))
		state = TRUE;

	if (result)
		g_free (result);

	return state;
}

osync_bool modify_item (OSyncContext *ctx, const char *type, unsigned int uid, const char *data, gchar **error)
{
        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("GPE_SYNC", 2, "Modifying item %d:\n%s", uid, data);
	gchar *result = NULL;
	osync_bool state = FALSE;
	gchar vtype[7] = "";

	if (!strcmp (type, "contact"))
		sprintf (vtype, "vcard");
	else if (!strcmp (type, "event"))
		sprintf (vtype, "vevent");
	else if (!strcmp (type, "todo"))
		sprintf (vtype, "vtodo");
			
	gpesync_client_exec_printf (env->client, "modify %s %d %s", client_callback_string, &result, error, vtype, uid, data);

	if (!strcasecmp (result, "OK\n"))
		state = TRUE;

	if (result)
		g_free (result);

	return state;
}
osync_bool delete_item (OSyncContext *ctx, const char *type, unsigned int uid, gchar **error)
{
        gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	osync_debug("GPE_SYNC", 2, "Deleting item %d", uid);
	gchar *result = NULL;
	osync_bool state = FALSE;
	gchar vtype[7] = "";

	if (!strcmp (type, "contact"))
		sprintf (vtype, "vcard");
	else if (!strcmp (type, "event"))
		sprintf (vtype, "vevent");
	else if (!strcmp (type, "todo"))
		sprintf (vtype, "vtodo");
			
	gpesync_client_exec_printf (env->client, "del %s %d", client_callback_string, &result, error, vtype, uid);

	if (!strcasecmp (result, "OK\n"))
		state = TRUE;

	if (result)
		g_free (result);

	return state;
}

osync_bool parse_uid_modified (gchar *string, gchar **uid, gchar **modified)
{
	gchar *c = NULL;
	*uid = string;
	*modified = (strchr (string, ':'));
	if (*modified == NULL)
		return FALSE;
	*modified += 1;
	
	c = strchr (*modified, '\n');
	if (c)
		c[0] = '\0';
	
	c = strchr (*uid, ':');
	c[0] = '\0';

	return TRUE;
}


osync_bool report_change (OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, gchar *data)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data (ctx);
	OSyncChange *change = osync_change_new ();
	osync_change_set_uid (change, uid);
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

	osync_change_set_hash (change, hash);
	osync_change_set_data (change, data, strlen (data), TRUE);
	
	if (osync_hashtable_detect_change (env->hashtable, change)) {
		osync_context_report_change (ctx, change);
		osync_hashtable_update_hash (env->hashtable, change);
	}

	return TRUE;
}
