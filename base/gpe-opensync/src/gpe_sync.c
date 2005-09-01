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

/*! \brief Initializes the plugin (needed for opensync)
 *
 * \param member	The member of the sync pair
 * \param error		If an error occurs it will be stored here
 */
static void *initialize(OSyncMember *member, OSyncError **error)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	char *configdata = NULL;
	int configsize = 0;
	
	gpe_environment *env = g_malloc0(sizeof(gpe_environment));
	assert(env != NULL);

	//now you can get the config file for this plugin
	if (!osync_member_get_config_or_default(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		g_free(env);
		osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: %s", __func__, osync_error_print(error));
		return NULL;
	}
	
	osync_debug("GPE_SYNC", 4, "configdata: %s", configdata);
	
	if (!gpe_parse_settings(env, configdata, configsize)) {
		osync_error_set(error, OSYNC_ERROR_MISCONFIGURATION, "Unable to parse plugin configuration for gpe plugin");
		g_free(env);
		osync_trace(TRACE_EXIT_ERROR, "GPE-SYNC %s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	// Here we set some default values
	env->member = member;
	env->client = NULL;
	
	//Set up a hash to detect changes
	env->hashtable = osync_hashtable_new();
	
	osync_trace(TRACE_EXIT, "GPE-SYNC %s: %p", __func__, env);

	return (void *)env;
}

/*! \brief Connects to the databases of GPE
 *
 * \param ctx		The context of the plugin
 */
static void connect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);

	// We need to get the context to load all our stuff.
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	OSyncError *error = NULL;

	gchar *path = g_strdup_printf ("%s@%s", env->username, env->device_addr);

	char *client_err;
	env->client = gpesync_client_open (path, &client_err);
	if (env->client == NULL) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, client_err);
		env->client = NULL;
		return;
	}
	
	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, osync_error_print(&error));

		return;
	}
	
	osync_context_report_success(ctx);

	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}

/* \brief Reports all the objects that were changes
 *
 * \param ctx		The context of the plugin
 */
static void get_changeinfo(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);

	osync_bool get_contacts = FALSE,
		   get_calendar = FALSE,
		   get_todo     = FALSE;
		   
	get_contacts = gpe_contacts_get_changes(ctx);
	get_calendar = gpe_calendar_get_changes(ctx);
	get_todo = gpe_todo_get_changes(ctx);

	//Now we need to answer the call (if all went fine)
	if (get_contacts && get_calendar && get_todo)
	{
		osync_context_report_success(ctx);
	}

	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}


/*! \brief This is called once all objects have been sent to the plugin
 *
 * \param ctx		The context of the plugin
 */
static void sync_done(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	//If we have a hashtable we can now forget the already reported changes
	osync_hashtable_forget(env->hashtable);
	
	//Answer the call
	osync_context_report_success(ctx);
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}

/*! \brief Closes the connection to the databases
 *
 * \brief ctx		The context of the plugin
 */
static void disconnect(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	//Close the hashtable
	osync_hashtable_close(env->hashtable);
	
	if (env->client) {
		gpesync_client_close (env->client);
		env->client = NULL;
	}

	//Answer the call
	osync_context_report_success(ctx);
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}

/*! \brief The counterpart to initialize
 * 
 * \param data		The data of the plugin (configuration, etc.)
 */
static void finalize(void *data)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	gpe_environment *env = (gpe_environment *)data;
	
	//Free all stuff that you have allocated here.
	osync_hashtable_free(env->hashtable);
	g_free(env->username);
	g_free(env->device_addr);

	if (env->client)
	  gpesync_client_close (env->client);
	
	g_free(env);
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}

/*! \brief This function has to be in every opensync plugin
 *
 * \brief env		The environment of the plugin containing basic
 * 			information about the plugin.
 */
void get_info(OSyncEnv *env)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	//Tell opensync something about your plugin
	info->name = "gpe-sync";
	info->longname = "Provides synchronisation with handhelds using GPE.";
	info->description = "See http://gpe.handhelds.org for more information";
	info->version = 1;
	info->is_threadsafe = TRUE;
	
	//Now set the function we made earlier
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	
	//If you like, you can overwrite the default timeouts of your plugin
	//The default is set to 60 sec. Note that this MUST NOT be used to
	//wait for expected timeouts (Lets say while waiting for a webserver).
	//you should wait for the normal timeout and return a error.
	info->timeouts.connect_timeout = 5;
	
	// Now we got to tell opensync, which formats and what types we accept
	gpe_contacts_setup (info);
	gpe_calendar_setup (info);
	gpe_todo_setup (info);
	
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}
