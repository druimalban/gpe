#include <opensync/opensync.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "gpe_sync.h"

/*! \brief Initializes the plugin (needed for opensync)
 *
 * \param member	???
 * \param error		???
 */
static void *initialize(OSyncMember *member, OSyncError **error)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	char *configdata = NULL;
	int configsize = 0;

	g_type_init();
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	gpe_environment *env = g_malloc0(sizeof(gpe_environment));
	assert(env != NULL);

	//now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
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

	//Process the configdata here and set the options on your environment
	env->member = member;
	OSyncGroup *group = osync_member_get_group(member);
	env->change_id = g_strdup(osync_group_get_name(group));

	env->contacts = NULL;
	
	//Set up a hash to detect changes
	env->hashtable = osync_hashtable_new();
	if (env->hashtable == NULL) {
		printf ("ERROR creating hashtable!\n");		
	}
	
	osync_trace(TRACE_EXIT, "GPE-SYNC %s: %p", __func__, env);

	//Now your return your struct.
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

	printf ("Connecting to: %s:%d user: %s\n",env->device_addr, env->port, env->username);
	OSyncError *error = NULL;

	if (gpe_contacts_connect(ctx) == FALSE) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "Could not connect to contacts.");
		return;
	}

	if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, osync_error_print(&error));

		return;
	}
	
	osync_context_report_success(ctx);

	/* TODO: What is this??
	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	//char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	if (!osync_anchor_compare(env->member, "lanchor", lanchor))
		osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
        */
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}

/* \brief Reports all the objects that were changes
 *
 * \param ctx		The context of the plugin
 */
static void get_changeinfo(OSyncContext *ctx)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);

	gpe_contacts_get_changes(ctx);
	
	//Now we need to answer the call
	osync_context_report_success(ctx);
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
	
	/*
	 * This function will only be called if the sync was successfull
	 */
	
	//If we have a hashtable we can now forget the already reported changes
	osync_hashtable_forget(env->hashtable);
	
	//If we use anchors we have to update it now.
	char *lanchor = NULL;
	//Now you get/calculate the current anchor of the device
	osync_anchor_update(env->member, "lanchor", lanchor);
	
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
	
	//Close all stuff you need to close
	
	//Close the hashtable
	osync_hashtable_close(env->hashtable);
	
	if (env->contacts) {
		gpe_contacts_disconnect(ctx);
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
	//the version of the api we are using, (1 at the moment)
	info->version = 1;
	//If you plugin is threadsafe.
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
	gpe_contacts_setup(info);
	
	osync_debug("GPE_SYNC", 4, "stop: %s", __func__);
}
