/* 
 * MultiSync GPE Plugin
 * Copyright (C) 2004 Phil Blundell <pb@nexus.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 */

#include <stdlib.h>
#include <errno.h>
#include <glib.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include "multisync.h"

#include "gpe_sync.h"

/* this should match MULTISYNC_API_VER in 
 * multisync.h if everything is up to date */
#define GPE_MULTISYNC_API_VER (3)

#define _(x)  gettext(x)

GSList *db_list;

/******************************************************************
   The following functions are called by the syncengine thread, and
   syncengine expects an asynchronous answer using on of

   // Success
   sync_set_requestdone(sync_pair*);
   // General failure (equal to sync_set_requestmsg(SYNC_MSG_REQFAILED,...))
   sync_set_requestfailed(sync_pair*);
   // General failure with specific log string
   sync_set_requestfailederror(char*, sync_pair*);
   // Success with data
   sync_set_requestdata(gpointer data, sync_pair*);
   // General return (with either failure or other request)
   sync_set_requestmsg(sync_msg_type, sync_pair*);
   // General return with log string
   sync_set_requestmsgerror(sync_msg_type, char*, sync_pair*);
   // General return with data pointere
   sync_set_requestdatamsg(gpointer data, sync_msg_type, sync_pair*);

   (Yes, there are lots of them for convenience.)
   These functions do not have to be called from this thread. If
   your client uses the gtk main loop, use gtk_idle_add() to call your
   real function and let that function call sync_set_requestsomething()
   when done.
******************************************************************/

/* sync_connect()

   This is called once every time the sync engine tries to get changes
   from the two plugins, or only once if always_connected() returns true.
   Typically, this is where you should try to connect to the device
   to be synchronized, as well as load any options and so on.
   The returned struct must contain a

   client_connection commondata; 

   first for common MultiSync data.

   NOTE: Oddly enough (for historical reasons) this callback MUST
   return the connection handle from this function call (and NOT by
   using sync_set_requestdata()). The sync engine still waits for a 
   sync_set_requestdone() call (or _requestfailed) before continuing.
*/
gpe_conn* 
sync_connect (sync_pair* handle, connection_type type,
	      sync_object_type object_types) 
{
  gpe_conn *conn = NULL;
  
  conn = g_malloc0 (sizeof (gpe_conn));
  g_assert (conn);
  conn->sync_pair = handle;
  conn->commondata.object_types = object_types;

  pthread_create (&conn->thread, NULL, gpe_do_connect, conn);

  return conn;
}


/* sync_disconnect()

   Called by the sync engine to free the connection handle and disconnect
   from the database client.
*/
void 
sync_disconnect (gpe_conn *conn) 
{
  GSList *i;
  sync_pair *sync_pair = conn->sync_pair;
      
  GPE_DEBUG(conn, "sync_disconnect");    
  
  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;

      gpe_disconnect (db);
    }

  /* cleanup memory from the connection */
  if (conn->device_addr)
    g_free (conn->device_addr);

  if (conn->username)
    g_free (conn->username);

  g_free (conn);  
  
  sync_set_requestdone (sync_pair);
}


/* get_changes()

   The most important function in the plugin. This function is called
   periodically by the sync engine to poll for changes in the database to
   be synchronized. The function should return a pointer to a gmalloc'ed
   change_info struct (which will be freed by the sync engine after usage).
   using sync_set_requestdata(change_info*, sync_pair*).
   
   For all data types set in the argument "newdbs", ALL entries should
   be returned. This is used when the other end reports that a database has
   been reset (by e.g. selecting "Reset all data" in a mobile phone.)
   Testing for a data type is simply done by 
   
   if (newdbs & SYNC_OBJECT_TYPE_SOMETHING) ...

   The "commondata" field of the connection handle contains the field
   commondata.object_types which specifies which data types should
   be synchronized. Only return changes from these data types.

   The changes reported by this function should be the remembered
   and rereported every time until sync_done() (see below) has been 
   called with a success value. This ensures that no changes get lost
   if some connection fails.  
*/

void 
get_changes (gpe_conn *conn, sync_object_type newdbs) 
{
  conn->newdbs = newdbs;

  pthread_create (&conn->thread, NULL, gpe_do_get_changes, conn);
}

/* syncobj_modify() 

   Modify or add an object in the database. This is called by the sync
   engine when a change has been reported in the other end.

   Arguments:
   object     A string containing the actual data of the object. E.g. for
              an objtype of SYNC_OBJECT_TYPE_CALENDAR, this is a 
	      vCALENDAR 2.0 string (see RFC 2445).
   uid        The unique ID of this entry. If it is new (i.e. the sync engine
              has not seen it before), this is NULL.
   objtype    The data type of this object.
   returnuid  If uid is NULL, then the ID of the newly created object should
              be returned in this buffer (if non-NULL). The length of the
	      ID should be returned in returnuidlen.
*/

void 
syncobj_modify (gpe_conn *conn, char* object, char *uid,
		sync_object_type objtype, char *returnuid, int *returnuidlen) 
{
  GError *err = NULL;
  GSList *i;

  GPE_DEBUG (conn, "syncobj_modify");  
  
  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;

      if (objtype & db->type)
	db->push_object (db, object, uid, returnuid, returnuidlen, &err);
    }
  
  sync_set_requestdone (conn->sync_pair);
}


/* syncobj_delete() 

   Delete an object from the database. If the argument softdelete is 
   true, then this object is deleted by the sync engine for storage reasons.
*/
void 
syncobj_delete (gpe_conn *conn, char *uid, 
		sync_object_type objtype, int softdelete) 
{
  gboolean soft = softdelete ? TRUE : FALSE;
  GSList *i;

  GPE_DEBUG (conn, "syncobj_delete");
   
  if (!uid)
  {
    GPE_DEBUG (conn, "item to delete not specified by syncengine"); 
    sync_set_requestfailed (conn->sync_pair); 
    return;
  }
  
  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;

      if (objtype & db->type)
	db->delete_object (db, uid, soft);
    }
  
  sync_set_requestdone (conn->sync_pair);
}


/* syncobj_get_recurring()

   This is a very optional function which may very well be removed in 
   the future. It should return a list of all recurrence instance of
   an object (such as all instances of a recurring calendar event).
   
   The recurring events should be returned as a GList of changed_objects
   with change type SYNC_OBJ_RECUR.
*/

void 
syncobj_get_recurring (gpe_conn *conn, changed_object *obj) 
{
  GPE_DEBUG(conn, "syncobj_get_recurring");      
  
  /* 
   * not implemented
   */
  
  sync_set_requestdata (NULL, conn->sync_pair);
}


/* sync_done()

   This function is called by the sync engine after a synchronization has
   been completed. If success is true, the sync was successful, and 
   all changes reported by get_changes can be forgot. If your database
   is based on a change counter, this can be done by simply saving the new
   change counter.
*/
void 
sync_done (gpe_conn *conn, gboolean success) 
{
  GSList *i;

  for (i = db_list; i; i = i->next)
    {
      struct db *db = i->data;

      if (db->changed)
	{
	  gchar *filename;
	  FILE *fp;

	  filename = g_strdup_printf ("%s/%s", 
				      sync_get_datapath (conn->sync_pair),
				      db->name);

	  fp = fopen (filename, "w");
	  if (fp)
	    {
	      fprintf (fp, "%d\n", (int)db->current_timestamp);
	      fclose (fp);
	    }
	  
	  g_free (filename);
	}
    }

  sync_set_requestdone (conn->sync_pair);
}


/***********************************************************************
 The following functions are synchronous, i.e. the syncengine
 expects an immedieate answer without using sync_set_requestsomething()
************************************************************************/

/* always_connected()
   Return TRUE if this client does not have to be polled (i.e. can be 
   constantly connected).  */
gboolean 
always_connected (void) 
{
  return FALSE;
}


/* short_name()
   Return a short plugin name for internal use.  */
char *
short_name (void)
{
  return "gpe";
}


/* long_name()
   Return a long name which can be shown to the user.  */
char *
long_name (void)
{
  return _("GPE");
}


/* plugin_info()
   Return an even longer description of what this plugin does. This will
   be shown next to the drop-down menu in the sync pair options.  */
char * 
plugin_info (void) 
{
  return _("This plugin allows you to synchronize with the GPE Palmtop Environment");
}

/* plugin_init()
   Initialize the plugin. Called once upon loading of the plugin (NOT
   once per sync pair).  */
void 
plugin_init (void) 
{
  calendar_init ();
  todo_init ();
  contacts_init ();
}


/* object_types()

   Return the data types this plugin can handle.  */
sync_object_type 
object_types (void)
{
  return SYNC_OBJECT_TYPE_CALENDAR | SYNC_OBJECT_TYPE_TODO | 
	 SYNC_OBJECT_TYPE_PHONEBOOK;
}


/* plugin_API_version()
   
   Return the MultiSync API version for which the plugin was compiled.
   It is defined in multisync.h as MULTISYNC_API_VER.
   Do not use return(MULTISYNC_API_VER), though, as the plugin will then
   get valid after a simple recompilation. This may not be all that is needed.  */
int 
plugin_API_version (void)
{
  return GPE_MULTISYNC_API_VER;
}
