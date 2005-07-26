#include <opensync/opensync.h>
#include "gpe_sync.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void *initialize(OSyncMember *member, OSyncError **error)
{
	osync_debug("GPE_SYNC", 4, "start: %s", __func__);
	char *configdata = NULL;
	int configsize = 0;
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	gpe_environment *env = malloc(sizeof(gpe_environment));
	assert(env != NULL);
	memset(env, 0, sizeof(gpe_environment));
	
	//now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		free(env);
		return NULL;
	}

	osync_debug("GPE_SYNC", 4, "configdata: %s", configdata);
	
	//Process the configdata here and set the options on your environment
	free(configdata);
	env->member = member;
	
	//If you need a hashtable you make it here
	//env->hashtable = osync_hashtable_new();
	
	//Now your return your struct.
	return (void *)env;
}

static void connect(OSyncContext *ctx)
{
	//Each time you get passed a context (which is used to track
	//calls to your plugin) you can get the data your returned in
	//initialize via this call:
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);

	/*
	 * Now connect to your devices and report
	 * 
	 * an error via:
	 * osync_context_report_error(ctx, ERROR_CODE, "Some message");
	 * 
	 * or success via:
	 * osync_context_report_success(ctx);
	 * 
	 * You have to use one of these 2 somewhere to answer the context.
	 * 
	 */
	
	//If you are using a hashtable you have to load it here
	//OSyncError *error = NULL;
	//if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
	//	osync_context_report_osyncerror(ctx, &error);
	//	return;
	//}
	
	//you can also use the anchor system to detect a device reset
	//or some parameter change here. Check the docs to see how it works
	char *lanchor = NULL;
	//Now you get the last stored anchor from the device
	if (!osync_anchor_compare(env->member, "lanchor", lanchor))
		osync_member_set_slow_sync(env->member, "<object type to request a slow-sync>", TRUE);
}

static void get_changeinfo(OSyncContext *ctx)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	//If you use opensync hashtables you can detect if you need
	//to do a slow-sync and set this on the hastable directly
	//otherwise you have to make 2 function like "get_changes" and
	//"get_all" and decide which to use using
	//osync_member_get_slow_sync
	//if (osync_member_get_slow_sync(env->member, "<object type>"))
	//	osync_hashtable_set_slow_sync(env->hashtable, "<object type>");

	/*
	 * Now you can get the changes.
	 * Loop over all changes you get and do the following:
	 */
		char *data = NULL;
		//Now get the data of this change
		
		//Make the new change to report
		OSyncChange *change = osync_change_new();
		//Set the member
		osync_change_set_member(change, env->member);
		//Now set the uid of the object
		osync_change_set_uid(change, "<some uid>");
		//Set the object format
		osync_change_set_objformat_string(change, "<the format of the object>");
		//Set the hash of the object (optional, only required if you use hashtabled)
		//osync_change_set_hash(change, "the calculated hash of the object");
		//Now you can set the data for the object
		//Set the last argument to FALSE if the real data
		//should be queried later in a "get_data" function
		
		osync_change_set_data(change, data, sizeof(data), TRUE);			

		//If you use hashtables use these functions:
		//if (osync_hashtable_detect_change(env->hashtable, change)) {
		//	osync_context_report_change(ctx, change);
		//	osync_hashtable_update_hash(env->hashtable, change);
		//}	
		//otherwise just report the change via
		//osync_context_report_change(ctx, change);

	//When you are done looping and if you are using hashtables	
	//osync_hashtable_report_deleted(env->hashtable, ctx, "data");
	
	//Now we need to answer the call
	osync_context_report_success(ctx);
}

static osync_bool commit_change(OSyncContext *ctx, OSyncChange *change)
{
	//gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	/*
	 * Here you have to add, modify or delete a object
	 * 
	 */
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			//Delete the change
			//Dont forget to answer the call on error
			break;
		case CHANGE_ADDED:
			//Add the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the hash here:
			//osync_change_set_hash(change, "new hash");
			break;
		case CHANGE_MODIFIED:
			//Modify the change
			//Dont forget to answer the call on error
			//If you are using hashtables you have to calculate the new hash here:
			//osync_change_set_hash(change, "new hash");
			break;
		default:
			osync_debug("FILE-SYNC", 0, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	//if you use hashtable, update the hash now.
	//osync_hashtable_update_hash(env->hashtable, change);
	return TRUE;
}

static void sync_done(OSyncContext *ctx)
{
	gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	/*
	 * This function will only be called if the sync was successfull
	 */
	
	//If we have a hashtable we can now forget the already reported changes
	//osync_hashtable_forget(env->hashtable);
	
	//If we use anchors we have to update it now.
	char *lanchor = NULL;
	//Now you get/calculate the current anchor of the device
	osync_anchor_update(env->member, "lanchor", lanchor);
	
	//Answer the call
	osync_context_report_success(ctx);
}

static void disconnect(OSyncContext *ctx)
{
	//gpe_environment *env = (gpe_environment *)osync_context_get_plugin_data(ctx);
	
	//Close all stuff you need to close
	
	//Close the hashtable
	//osync_hashtable_close(env->hashtable);
	//Answer the call
	osync_context_report_success(ctx);
}

static void finalize(void *data)
{
	//gpe_environment *env = (gpe_environment *)data;
	//Free all stuff that you have allocated here.
	//osync_hashtable_free(env->hashtable);
}

void get_info(OSyncEnv *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info(env);
	
	//Tell opensync something about your plugin
	info->name = "GPE-plugin";
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
	//There are more timeouts for the other functions
	
	//Now you have to tell opensync all the object types that your are
	//accepting. This can be more than one
	//osync_plugin_accept_objtype(info, "<object type name>");
	//which format do you accept for this objtype
	//osync_plugin_accept_objformat(info, "<object type name>", "<format name>", "<name of the required extension if any>");
	//set the commit function for this format
	//osync_plugin_set_commit_objformat(info, "<object type name>", "<format name>", commit_change);
	
	//All the stuff above will be made through these functions:
	gpe_adresses_setup(info);
	osync_plugin_set_commit_objformat(info,"contact", "file",commit_change);
}
