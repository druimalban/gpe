#ifndef MULTISYNC_H
#define MULTISYNC_H

// Part of the MultiSync synchronization engine API.

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <pthread.h>

// A plugin must conform to this API version to work.
// Version 3 == version 2 + gnome2
#define MULTISYNC_API_VER 3

/* macro for wrapping debug print outs */
#define dd(x) (multisync_debug?(x):0)

typedef enum {
  SYNC_OBJECT_TYPE_UNKNOWN = 0x00,
  SYNC_OBJECT_TYPE_CALENDAR = 0x01,
  SYNC_OBJECT_TYPE_PHONEBOOK = 0x02,
  SYNC_OBJECT_TYPE_TODO = 0x04,
  SYNC_OBJECT_TYPE_ANY = 0xff
} sync_object_type;


typedef enum {
  SYNC_MSG_NOMSG = 0,
  SYNC_MSG_OBJECTCHANGE = 1,
  SYNC_MSG_REQDONE = 2,
  SYNC_MSG_SLOWSYNC = 3,
  SYNC_MSG_TRUE = 4,
  SYNC_MSG_FALSE = 5,
  SYNC_MSG_FORCESYNC = 6,
  SYNC_MSG_QUIT = 7,
  SYNC_MSG_GET_LOCAL_CHANGES = 8,
  SYNC_MSG_GET_REMOTE_CHANGES = 9,
  SYNC_MSG_LOCAL_MODIFY = 10,
  SYNC_MSG_REMOTE_MODIFY = 11,
  SYNC_MSG_LOCAL_SYNCDONE = 12,
  SYNC_MSG_REMOTE_SYNCDONE = 13,
  SYNC_MSG_FORCERESYNC = 14,
  SYNC_MSG_REQFAILED = -1,
  SYNC_MSG_CONNECTIONERROR = -2,
  SYNC_MSG_MODIFYERROR = -3,
  SYNC_MSG_PROTOCOLERROR = -4,
  SYNC_MSG_NORIGHTSERROR = -5,
  SYNC_MSG_DONTEXISTERROR = -6,
  SYNC_MSG_DATABASEFULLERROR = -7,
  SYNC_MSG_USERDEFERRED = -8
} sync_msg_type;

typedef enum {
  VOPTION_CONVERTUTC = 0x0001, // Convert UTC times to local time
  VOPTION_ADDUTF8CHARSET = 0x0002, // Add CHARSET=UTF-8 when unspecified
  VOPTION_FIXDSTTOCLIENT = 0x0004, // Fix a specific DST problem
  VOPTION_FIXDSTFROMCLIENT = 0x0008, // Fix a specific DST problem
  VOPTION_FIXCHARSET = 0x0010, // Convert from specified charset when unspec
  VOPTION_FIXTELOTHER = 0x0020, // Convert TEL: to TEL;VOICE: for Evolution
  VOPTION_CALENDAR2TO1 = 0x0040, // Convert vCALENDAR 2.0 to 1.0
  VOPTION_CALENDAR1TO2 = 0x0080, // Convert vCALENDAR 1.0 to 2.0
  VOPTION_REMOVEALARM = 0x0100, // Remove alarms
  VOPTION_REMOVEPHOTO = 0x0200 // Remove photo from vCARD
} sync_voption;


#define SYNC_OBJ_MODIFIED 1
#define SYNC_OBJ_ADDED 2
#define SYNC_OBJ_SOFTDELETED 3
#define SYNC_OBJ_HARDDELETED 4
#define SYNC_OBJ_RECUR 5
// Used if the object is caught in a filter
#define SYNC_OBJ_FILTERED 6

typedef enum {
  SYNC_DIR_LOCALTOREMOTE = 1,
  SYNC_DIR_REMOTETOLOCAL = 2
} sync_direction;

// A call to syncobj_modify_list() must return a list of 
// syncobj_modify_result's. The list will be freed by the sync engine. 
// The list order is the same as the list of changed_object's sent to 
// syncobj_modify_list().
typedef struct {
  sync_msg_type result; // The result of the operation
  char *returnuid; // Return the UID for e.g. a new entry (may be NULL if N/A)
                   // as a gmalloc'ed string.
} syncobj_modify_result;

typedef struct {
  char *comp; // The data associated to this object type:
              // This data should be freeable using g_free().
              // May be NULL if the event is deleted.
  char *uid;  // The unique ID of this object
  char *removepriority; // Word to sort on for removal when database is full,
                        // usually the date when the entry ends. May be NULL.
  int change_type; // One of the above (SYNC_OBJ_MODIFIED, etc)
  sync_object_type object_type; //The data type of this object
} changed_object;

typedef struct {
  GList *changes; // List of changed_object's
  sync_object_type newdbs; // Set the bit for the corresponding type
  // if the database is not recognized or has been reset.
} change_info;

// sync_pair contains a lot of data, but plugins should not use it directly.
typedef void sync_pair;

typedef struct  {
  int calendarrecords;
  int maxcalendarrecords;
  int todorecords;
  int maxtodorecords;
  int phonebookrecords;
  int maxphonebookrecords;
  gboolean fake_recurring;
  gboolean managedbsize;
  sync_object_type object_types;
  gboolean is_feedthrough;
} client_connection;

typedef enum {
  CONNECTION_TYPE_LOCAL,
  CONNECTION_TYPE_REMOTE
} connection_type;

typedef enum {
  SYNC_LOG_SUCCESS = 0,
  SYNC_LOG_ERROR = 1
} sync_log_type;


/**********************************************************************
  MultiSync communication:
 **********************************************************************/
// Tell MultiSync that we have detected a change in the data
void sync_object_changed(sync_pair *thissync);
// Force synchronization (method called by e.g. "Sync Now" button)
void sync_force_sync(sync_pair *thissync);

/**********************************************************************
  Asynchronous return functions
 **********************************************************************/
// For all asynchronous function calls (almost all), the return is done
// through one of the following functions:
// Return success
void sync_set_requestdone(sync_pair *thissync);
// Return general failure
gpointer sync_set_requestfailed(sync_pair *thissync);
// Return failure with error string
gpointer sync_set_requestfailederror(char* errorstr, sync_pair *thissync);
// Return success with data
void sync_set_requestdata(gpointer data,sync_pair *thissync);
// Return specific return message
void sync_set_requestmsg(sync_msg_type type,sync_pair *thissync);
// Return specific message with error string
void sync_set_requestmsgerror(sync_msg_type type, char *errorstr, 
			      sync_pair *thissync);
// Return specific message with data
void sync_set_requestdatamsg(gpointer data, sync_msg_type type, 
			     sync_pair *thissync);


/**********************************************************************
  Functions for feedthrough plugins
 **********************************************************************/
// For a feedthrough plugin, tell MultiSync to get changes from other plugin
void sync_feedthrough_get_changes(sync_pair *thissync, connection_type type,
				  sync_object_type newdbs);
// For a feedthrough plugin, tell MultiSync to modify
void sync_feedthrough_modify(sync_pair *thissync, connection_type type,
			     GList *modify);
// For a feedthrough plugin, tell MultiSync sync is done (must be
// called if sync_feedthrough_get_changes() has been called)
void sync_feedthrough_syncdone(sync_pair *thissync, connection_type type,
			       gboolean success);

/**********************************************************************
  Utility functions
 **********************************************************************/
// Free a change list including data and GList
void sync_free_changes(GList *changes);
// Make a copy of a changed_object
changed_object* sync_copy_changed_object(changed_object *change);
// Free a changed_object
void sync_free_changed_object(changed_object *change);
// Free the whole change_info
void sync_free_change_info(change_info *change);
// Free a list of syncobj_modify_result's
void sync_free_modify_results(GList *results);
// Function to when the plugin option window is closed.
void sync_plugin_window_closed();
// Return the object type as a string
char* sync_objtype_as_string(sync_object_type objtype);
// Convert sync_msg_type to error string (do not free after use)
char *sync_error_string(sync_msg_type err);
// Add some string to the log
void sync_log(sync_pair *pair, char* logstring, sync_log_type type);
// Set the status message of the sync pair
void sync_set_pair_status(sync_pair *pair, char *status);
// Get a specific line of data from a vCARD, vCALENDAR etc.
char* sync_get_key_data(char *card, char *key);
// Do conversions and various "fixes" to vCARD's and vCALENDARS
char* sync_vtype_convert(char *card, sync_voption opts, char *charset);

// Get the filesystem path to the directory for this sync pair 
// (for storing options etc)
char* sync_get_datapath(sync_pair *pair);

#endif
