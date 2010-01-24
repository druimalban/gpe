/*
 * Copyright (C) 2007-2008 Christoph Würstle <n800@axique.de>
 * Copyright (C) 2008-2010 Graham R. Cobb <g+gpe@cobb.uk.net>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//
//gcc `pkg-config --cflags --libs gtk+-2.0 hildon-libs dbus-1 libosso libeventdb libtododb libcontactsdb` main.c -o main
//


#include <config.h>

#include <glib.h>
#include <stdlib.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkbutton.h>

#include <gpe/event-db.h>
#include <gpe/pixmaps.h>
#include <gpe/init.h>
#include <gpe/pim-categories.h>
#include <gpe/todo-db.h>
#include <gpe/contacts-db.h>

#include <libintl.h>
#include <locale.h>


/* Hildon includes */
#if HILDON_VER > 0
#include <hildon/hildon-program.h>
#include <hildon/hildon-window.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-banner.h>
#else
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-home-plugin/hildon-home-plugin-interface.h>
#include <hildon-widgets/hildon-note.h>
#endif /* HILDON_VER > 0 */

#include <libosso.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <config.h>

#define APPLICATION_DBUS_SERVICE "gpesummary"
#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"


#define GPE_DIR_ "/.gpe"
#define GPE_DIR() \
  g_strdup_printf ("%s" GPE_DIR_, g_get_home_dir ())

#define CALENDAR_FILE_ "/calendar"
#define CALENDAR_FILE() \
  g_strdup_printf ("%s" GPE_DIR_ CALENDAR_FILE_, g_get_home_dir ())
gchar *calendar_file = NULL;
GnomeVFSMonitorHandle *monitor_calendar = NULL;

#define TODO_FILE_ "/todo"
#define TODO_FILE() \
  g_strdup_printf ("%s" GPE_DIR_ TODO_FILE_, g_get_home_dir ())
gchar *todo_file = NULL;
GnomeVFSMonitorHandle *monitor_todo = NULL;
 
#define _(String) dgettext (PACKAGE,String)

#define WIDTH 300
#define HEIGHT 400

gboolean is_visible;

GtkWidget *mainwidget = NULL;
GtkWidget *mainvbox = NULL;
GtkWidget *headtitle = NULL;
GtkWidget *widget_window = NULL;
time_t last_gui_update;

gboolean doshow_birthdays; /* Whether we are currently showing birthdays */
gboolean show_birthdays_pref; /* Whether the user wants us to try to show birthdays */
gboolean doshow_appointments;
gboolean doshow_todos;
gboolean doshow_alltodos;

gboolean doshow_buttons;
gint doshow_countitems;
//gboolean doshow_autorefresh;
gboolean doshow_extended; //waste space
gboolean doshow_vexpand; //no option yet!

gboolean refresh_now; //User asked for refresh update_clock manages this

static osso_context_t *osso = NULL;

GSList *birthdaylist = NULL;
gchar lastGPEDBupdate[6];
int todocount;

guint current_timer = 0;

EventDB *event_db = NULL;

static void reset_globals() {
  /* Initialise globals and reset options to default */
  /* Note: this is to handle re-adding the widget to the home 
     screen without reloading the code.  No pointers to allocated
     memory should be reset unless the memory concerned has been
     freed, otherwise memory loss will occur.  To avoid bugs, pointers
     should be reset at the same time as the memory concerned is
     freed, not here */

  g_message("%s",__func__);

  is_visible = TRUE;

  last_gui_update = 0;

  doshow_birthdays = TRUE; /* Whether we are currently showing birthdays */
  show_birthdays_pref = TRUE; /* Whether the user wants us to try to show birthdays */
  doshow_appointments = TRUE;
  doshow_todos = TRUE;
  doshow_alltodos = TRUE;

  doshow_buttons = FALSE;
  doshow_countitems = 8;
  // doshow_autorefresh = FALSE;
  doshow_extended = TRUE; //waste space
  doshow_vexpand = FALSE; //no option yet!

  refresh_now = FALSE; //User asked for refresh update_clock manages this

  lastGPEDBupdate[0] = '\0';
  todocount = 0;
}

void printTime(gchar * comment) {
  g_message(comment);
/*
     struct tm tm;
     struct timeval tv;
     gettimeofday(&tv, NULL);
     tm=*localtime(&tv.tv_sec);
     printf("%s %d:%d:%d %d \n",comment, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
*/
}

static void async_cb(const gchar *interface, const gchar *method,
                     osso_rpc_t *retval, gpointer data)
{
        g_warning("method '%s' returned\n", method);
}

void todo_gpestart(GtkWidget *button, gpointer user_data) {
	g_message("starting gpe-todo");
	osso_return_t ret = osso_rpc_async_run(osso,
                                 "com.nokia.gpe_todo",
                                 "/com/nokia/gpe_todo",
                                 "com.nokia.gpe_todo",
                                 "top_application", async_cb, NULL,
                                 DBUS_TYPE_STRING,
                                 "this is the top_application parameter",
                                 DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
                g_warning("%s: osso_rpc_async_run returned %d",__func__,ret);
        }
}

static void todo_clicked( GtkWidget      *button, 
					GdkEventButton *event,
					gpointer user_data ) {
	//g_message(gtk_widget_get_name(button));	

  if (todo_db_start () != 0) {
    g_message("todo_db_start returned error");
    return;
  }
	
	GSList *iter;

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		g_message(gtk_widget_get_name(button));
		g_string_new( gtk_widget_get_name(button) );
		//g_string_append label->append(" (!)");
		if (strcmp(((struct todo_item *)iter->data)->todoid,gtk_widget_get_name(button))==0) {
			((struct todo_item *)iter->data)->state = COMPLETED;
			g_message("set to completed");
			todo_db_push_item(((struct todo_item *)iter->data));
			
			//todo_db_delete_item(((struct todo_item *)iter->data));
		}
	}
	
	todo_db_stop();
	GtkWidget *hbox = gtk_widget_get_parent(button);
	GtkWidget *vbox = gtk_widget_get_parent(hbox);
	gtk_widget_destroy(hbox);
	todocount--;
	g_message("todocount %i",todocount);
	if (todocount==0) {
	  GtkWidget *button;
	  g_message("adding no todos");
	  GtkWidget *eventbox;
	  eventbox = gtk_event_box_new();
	  gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
	  gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
	  gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
	  gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
	  button = gtk_label_new_with_mnemonic(_("(no todos)"));
	  gtk_container_add(GTK_CONTAINER(eventbox),button);
	  gtk_misc_set_alignment(GTK_MISC(button),0,0);
	  g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );
	  gtk_widget_show_all(GTK_WIDGET(vbox));
	}	
		
	g_slist_free(iter);
}


void calendar_gpestart(GtkWidget *button, gpointer user_data) {
	g_message("starting gpe-calendar");
	osso_return_t ret = osso_rpc_async_run(osso,
                                 "com.nokia.gpe_calendar",
                                 "/com/nokia/gpe_calendar",
                                 "com.nokia.gpe_calendar",
                                 "top_application", async_cb, NULL,
                                 DBUS_TYPE_STRING,
                                 "this is the top_application parameter",
                                 DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
                g_warning("%s: osso_rpc_async_run returned %d",__func__,ret);
        }	
}

void contacts_gpestart(GtkWidget *button, gpointer user_data) {
	g_message("starting gpe-contacts");
	osso_return_t ret = osso_rpc_async_run(osso,
                                 "com.nokia.gpe_contacts",
                                 "/com/nokia/gpe_contacts",
                                 "com.nokia.gpe_contacts",
                                 "top_application", async_cb, NULL,
                                 DBUS_TYPE_STRING,
                                 "this is the top_application parameter",
                                 DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
                g_warning("%s: osso_rpc_async_run returned %d",__func__,ret);
        }
}


static gboolean start_clock(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	osso_return_t ret = osso_rpc_async_run(osso,
                                 "com.nokia.osso_worldclock",
                                 "/com/nokia/osso_worldclock",
                                 "com.nokia.osso_worldclock",
                                 "top_application", async_cb, NULL,
                                 DBUS_TYPE_STRING,
                                 "this is the top_application parameter",
                                 DBUS_TYPE_INVALID);
        if (ret != OSSO_OK) {
                g_warning("%s: osso_rpc_async_run returned %d",__func__,ret);
                return 1;
        }
	return FALSE;
}


static gboolean todo_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	todo_gpestart(button, user_data);
	return FALSE;
}

static gboolean events_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	g_message("events_startclicked");
	calendar_gpestart(button, user_data);
	return FALSE;
}

static gboolean contacts_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	contacts_gpestart(button, user_data);
	return FALSE;
}

static void
alarm_fired (EventDB *edb, Event *ev)
{
  time_t tt = event_get_start(ev);
  g_message("Alarm fired - %s",ctime(&tt));

	calendar_gpestart(NULL,NULL);
}

static gboolean
alarms_process_pending (gpointer data)
{
  // Do nothing if Event DB not open
  if (event_db == NULL) return FALSE;

  g_signal_connect (G_OBJECT (event_db), "alarm-fired",
                    G_CALLBACK (alarm_fired), NULL);
  GSList *list = event_db_list_unacknowledged_alarms (event_db, NULL);
  list = g_slist_sort (list, event_alarm_compare_func);
  GSList *i;
  for (i = list; i; i = g_slist_next (i))
    alarm_fired (event_db, EVENT (i->data));
  event_list_unref (list);

  /* Don't run again.  */
  return FALSE;
}

gint show_todos(GtkWidget *vbox, gint count) {
        struct tm tm;
	if (doshow_todos==FALSE) return count;
	time_t tmp = time (NULL);
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&tmp);
	time_t todaystop = time (NULL)+(23-tm.tm_hour)*3600+(60-tm.tm_min)*60;
	time_t todaystart = time (NULL)-tm.tm_hour*3600-tm.tm_min*60-tm.tm_sec;
	todocount=0;
	
	if (todo_db_start () != 0) {
	  g_message("todo_db_start returned error");
	  return count;
	}

	GSList *iter;

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		if (((struct todo_item *)iter->data)->state == COMPLETED
		 || ((struct todo_item *)iter->data)->state == ABANDONED)
			continue;
		if (((struct todo_item *)iter->data)->time > todaystop)
			continue;
		if ((((struct todo_item *)iter->data)->time == 0)&&(doshow_alltodos==FALSE))
			continue;
		todocount+=1;
		
		GString *label = g_string_new( (((struct todo_item *)iter->data)->summary) );
		if ((((struct todo_item *)iter->data)->time < todaystart)&&(((struct todo_item *)iter->data)->time>0)) {
			g_string_append(label," (!)");
		}

		GtkWidget *button;
		GtkWidget *hbox_todo;
		GtkWidget *eventbox;
		hbox_todo = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox),hbox_todo,doshow_vexpand,doshow_vexpand,0);

		button = gtk_check_button_new();
		gtk_box_pack_start(GTK_BOX(hbox_todo),button,doshow_vexpand,doshow_vexpand,0);
		gtk_button_set_alignment(GTK_BUTTON(button),0.5,0.5);
		gtk_widget_set_name(button, ((struct todo_item *)iter->data)->todoid);	
		
		g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( todo_clicked ), NULL );
		
		eventbox= gtk_event_box_new();
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
		gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);

		gtk_box_pack_start(GTK_BOX(hbox_todo),eventbox,doshow_vexpand,doshow_vexpand,0);
		
		button = gtk_label_new(label->str);
		gtk_container_add(GTK_CONTAINER(eventbox),button);//,TRUE,TRUE,0);
		gtk_misc_set_alignment(GTK_MISC(button),0,0.5);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_startclicked ), NULL );	
	}
	
	if (todocount==0) {
		//g_message("todocount 0");
		GtkWidget *eventbox;
		GtkWidget *button;
		eventbox = gtk_event_box_new();
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
		gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
		gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
		button = gtk_label_new_with_mnemonic(_("(no todos)"));
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );

		
		todocount+=1;
	}	

	todo_db_stop ();
	
	g_slist_free(iter);
	gtk_widget_show_all(GTK_WIDGET(vbox));
	g_message("todosjow finished");
	return todocount+count;
}

void show_title(GtkWidget *vbox,gchar *title) {
	if (title!=NULL) {
		GtkWidget *button;
		button = gtk_label_new("");
		gtk_label_set_markup(GTK_LABEL(button),title);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		gtk_box_pack_start(GTK_BOX(vbox),button,doshow_vexpand,doshow_vexpand,0);
	}
}

/*
Because searching for birthdays is slow (have to check all contacts one by one,
make sure the contact-loop just needed to be searched through one time, and remember the uids
*/
void addBirthdaysAtDay(gchar *day) {
	GSList *iter;

	struct contacts_person *p;	

	for (iter = contacts_db_get_entries_finddlg (day,""); iter; iter = g_slist_next(iter)) {
		struct contacts_tag_value *ctv;
		p = contacts_db_get_by_uid(((struct contacts_person *)iter->data)->id);
		if (p) {
		  ctv=contacts_db_find_tag(p,"BIRTHDAY");
		} else ctv=NULL;

		if (ctv) {
			gboolean found=FALSE;
			if (strncmp((ctv->value)+4, day,4)==0) found=TRUE;
			
			if(found==TRUE) {
				int i =((struct contacts_person *)iter->data)->id;
				birthdaylist = g_slist_prepend (birthdaylist, GINT_TO_POINTER (i));
			}
			
		}
		while (gtk_events_pending()) { gtk_main_iteration(); }
	}
	g_slist_free(iter);
}

void prepare_birthdays() {
        struct tm tm;
	if (doshow_birthdays==FALSE) return;

	char day1[5],day2[5],day3[5],day4[5],day5[5],day6[5],day7[5];
	time_t start = time (NULL);
	memset (&tm, 0, sizeof (tm));
	//memset (&day1, 0, sizeof (day1));

	tm=*localtime(&start);	
	strftime (day1, sizeof(day1), "%m%d", &tm);

	g_slist_free(birthdaylist);
	birthdaylist=NULL;

	start=start+3600*24;
	tm=*localtime(&start);	
	strftime (day2, sizeof(day2), "%m%d", &tm);
	start=start+3600*24;
	tm=*localtime(&start);	
	strftime (day3, sizeof(day3), "%m%d", &tm);
	start=start+3600*24;
	tm=*localtime(&start);		
	strftime (day4, sizeof(day4), "%m%d", &tm);
	start=start+3600*24;
	tm=*localtime(&start);	
	strftime (day5, sizeof(day5), "%m%d", &tm);
	start=start+3600*24;
	tm=*localtime(&start);	
	strftime (day6, sizeof(day6), "%m%d", &tm);
	start=start+3600*24;
	tm=*localtime(&start);	
	strftime (day7, sizeof(day7), "%m%d", &tm);

	//g_message( buf2 );

	
	addBirthdaysAtDay(day1);
	addBirthdaysAtDay(day2);
	addBirthdaysAtDay(day3);
	addBirthdaysAtDay(day4);
	addBirthdaysAtDay(day5);
	addBirthdaysAtDay(day6);
	addBirthdaysAtDay(day7);
	while (gtk_events_pending()) { gtk_main_iteration(); }
}

gboolean show_birthdays (GtkWidget *vbox,time_t start,gchar *title) {
        struct tm tm;

	if (doshow_birthdays==FALSE) return TRUE;
	 
	char buf2[5];
	memset (&tm, 0, sizeof (tm));
	memset (&buf2, 0, sizeof (buf2));
	tm=*localtime(&start);
	strftime (buf2, sizeof(buf2), "%m%d", &tm);
	
	gboolean titletoshow=TRUE;
	GSList *iter;
	struct contacts_person *p;
	struct contacts_tag_value *ctv =NULL;
	
	//g_message("birthdaylist reading %p",&birthdaylist);
	
	for (iter = birthdaylist; iter; iter = g_slist_next(iter)) {
		guint id = (guint)iter->data;
		g_message("birthday id %i",id);
		p = contacts_db_get_by_uid(id);
		ctv=contacts_db_find_tag(p,"BIRTHDAY");
		
		if (strncmp(buf2,(ctv->value)+4,4)==0) {
		  GtkWidget *button;
				g_message(ctv->value);
				if (titletoshow==TRUE) show_title(vbox,title);
				titletoshow=FALSE;
				GString *label = g_string_new( _(" Birthday "));
				ctv=contacts_db_find_tag(p,"NAME");
				g_string_append(label,ctv->value);
				g_message( label->str );

				GtkWidget *eventbox= gtk_event_box_new();
				gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
				gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
				gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
				gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);
		
				button = gtk_label_new_with_mnemonic(label->str);  
				gtk_container_add(GTK_CONTAINER(eventbox),button);
				gtk_misc_set_alignment(GTK_MISC(button),0,0);
				g_signal_connect( G_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( contacts_startclicked ), NULL );	


		}

			
	}
	
	g_slist_free(iter);
	return titletoshow;
}


gint add_events(GtkWidget *vbox,EventDB *event_db, time_t start, time_t stop, gchar *title, gboolean showtitle, gint count) {
        struct tm tm;
	
	if (doshow_appointments==FALSE) return count;
	if (!event_db) return count;
	
	memset (&tm, 0, sizeof (tm));

	gboolean allDayEvent = FALSE;	

	GError **error = NULL;
	GSList *events = event_db_list_for_period (event_db, start ,stop, error );
	events = g_slist_sort (events, event_compare_func);
	GSList *iter;
	for (iter = events; iter; iter = iter->next)
	{

		Event *ev = EVENT (iter->data);
		
		char buf[200];
		time_t tt=event_get_start(ev);
		tm=*localtime(&tt);

		//Check for all day event
		strftime (buf, sizeof (buf), " %H:%M", &tm); //%d.%m
		allDayEvent=FALSE;
		if (strncmp(buf,(" 00:00"),6)==0) { allDayEvent=TRUE; }

		if (doshow_extended==FALSE) {
		  showtitle=FALSE;
		  strftime (buf, sizeof (buf), "%a %H:%M", &tm); //%d.%m 
		}

		if (doshow_extended==TRUE) {
			strcat(buf,"-");
		
			char buf2[20];
			tt=event_get_start(ev)+event_get_duration(ev);
			tm=*localtime(&tt);
			strftime (buf2, sizeof (buf2), "%H:%M", &tm); //%d.%m 		
			strcat(buf,buf2);
		}
		
		gboolean ignoreEvent = FALSE;
		if (allDayEvent == TRUE) {
			memset (&buf, 0, sizeof (buf)); //Allday event!
			if (doshow_extended==FALSE) {
		  	  showtitle=FALSE;
			  memset (&tm, 0, sizeof (tm));
		 	  tm=*localtime(&start);
		  	  strftime (buf, sizeof (buf), "%a", &tm); //%d.%m 
			}
		} else {
		  // Ignore events which start before the start time (currently in progress)
		  // or after the end time (should not happen?).
		  if (event_get_start(ev) < start) ignoreEvent=TRUE;
		  if (event_get_start(ev) > stop) ignoreEvent=TRUE;
		}
		strcat(buf," ");
		  

		if (ignoreEvent==FALSE) {
			
			count+=1;
			if (showtitle==TRUE) {
				showtitle=FALSE;
				show_title(vbox,title);
			}			
			
			strcat(buf,event_get_summary(ev, error) );
		
			GtkWidget *eventbox= gtk_event_box_new();
			gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
			gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
			gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
			gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);
			gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);		
			GtkWidget *button;
			button = gtk_label_new_with_mnemonic(buf);  
			gtk_container_add(GTK_CONTAINER(eventbox),button);
			gtk_misc_set_alignment(GTK_MISC(button),0,0);
			g_signal_connect( G_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( events_startclicked ), NULL );
		}
	}
	
	//show today for sure
	if ((count==0)&&(doshow_extended==TRUE)) {
		if (showtitle==TRUE) {
			showtitle=FALSE;
			show_title(vbox,title);
		}
		
		GtkWidget *eventbox= gtk_event_box_new();
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_event_box_set_above_child(GTK_EVENT_BOX(eventbox), TRUE);
		gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
		gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
		GtkWidget *button;
		button = gtk_label_new_with_mnemonic(_(" (no appointments)"));
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( events_startclicked ), NULL );
		count+=1;
	}
	
	g_slist_free(events);
	g_slist_free(iter);
	return count;
}


gint show_events(GtkWidget *vbox, gint count) {
        struct tm tm;
	time_t start = time (NULL); // Now
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&start);
	time_t stop = start + (23-tm.tm_hour)*3600+(59-tm.tm_min)*60+59-tm.tm_sec; // End of today
	
	/*char buf[200];
	tm=*localtime(&stop);
	strftime (buf, sizeof (buf), "%d %H:%M %S", &tm); //%d.%m 
	g_message("endtime: %s", buf);*/
	
	// Note we re-open the event DB (if we can find it), even if we are not showing appointments
	if (event_db) {
	  g_object_unref(event_db);
	  event_db = NULL;
	}
	GError *error = NULL;
	// If the event DB doesn't exist, event_db_new will return NULL
	event_db = event_db_readonly (calendar_file, &error);

	if (doshow_birthdays==TRUE) {
	  if (contacts_db_open(FALSE) != 0) doshow_birthdays = FALSE;
	}
	prepare_birthdays();

	//Today already set as head	
	gboolean titletoshow=show_birthdays(vbox,start,NULL);
	count+=add_events(vbox,event_db,start, stop, NULL, titletoshow,0);

	//titletoshow=show_birthdays(vbox,stop+2,"Tomorrow");
	//count=add_events(vbox,event_db,stop+2,stop+3600*24-2,"Tomorrow",titletoshow, count);
		
	//g_message("Tomorrow %i",count);
	stop++; // Tomorrow
	while ((count<doshow_countitems)&&(stop<start+3600*24*6)) {
		char buf2[20];
		//g_message("Tomorrow %i",count);
		memset (&tm, 0, sizeof (tm));
		memset (&buf2, 0, sizeof (buf2));
		tm=*localtime(&stop);
	
		strftime (buf2, sizeof(buf2), "<i>%A</i>", &tm);
		
		titletoshow=show_birthdays(vbox,stop+20,buf2);
		count=add_events(vbox,event_db,stop,stop+3600*24-1,buf2,titletoshow, count);
		stop=stop+3600*24;
	}

	if (doshow_birthdays==TRUE) contacts_db_close ();
	printTime("Events finished");
	g_idle_add (alarms_process_pending, NULL);
	return count;
	
}

void loadPrefs(){
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError **error = NULL;
        GError *error2 = NULL;

  g_message("%s",__func__);
  
	/* Create a new GKeyFile object and a bitwise list of flags. */
	keyfile = g_key_file_new ();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
  
	/* Load the GKeyFile from keyfile.conf or return. */
	GString *label = g_string_new( g_get_home_dir() );
	g_string_append(label,"/.gpesummary");

	g_message ("load_prefs 2");

	if (!g_key_file_load_from_file (keyfile, label->str , flags, &error2)) {
		g_warning ("failed to open conffile");
		g_warning (error2->message);
		g_message ("failed to open conffile");
		return ;
	}
 
	g_message ("load_prefs 3");

	/* Read in data from the key file from the group "username". */
	show_birthdays_pref = g_key_file_get_boolean(keyfile, "options","show_birthdays", error);
	doshow_birthdays = show_birthdays_pref;
	doshow_appointments = g_key_file_get_boolean(keyfile, "options","show_appointments", error);
	doshow_todos = g_key_file_get_boolean(keyfile, "options","show_todos", error);
	doshow_alltodos = g_key_file_get_boolean(keyfile, "options","show_alltodos", error);
	doshow_buttons = g_key_file_get_boolean(keyfile, "options","show_buttons", error);
	//doshow_autorefresh = g_key_file_get_boolean(keyfile, "options","show_autorefresh", error);
	doshow_countitems = g_key_file_get_integer(keyfile, "options", "show_countitems", error);
	doshow_extended = g_key_file_get_boolean(keyfile, "options","show_extended", error);

	g_message ("load_prefs 4");
	g_key_file_free (keyfile);
	g_string_free(label,TRUE);

	g_message ("load_prefs 5");
}


void show_all() {
  g_message("%s",__func__);
	last_gui_update=time(NULL);
	loadPrefs();
	g_message ("show_all 2");
	GtkWidget *vbox_todo;
	GtkWidget *vbox_events;
	GtkWidget *button;
	
	
	if (mainvbox!=NULL) gtk_widget_destroy(mainvbox);
	
	mainvbox = gtk_vbox_new (FALSE, 0);
#if MAEMO_VERSION_MAJOR >= 5
	gtk_container_add(GTK_CONTAINER(widget_window), mainvbox);
#else
	gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(widget_window), mainvbox );
#endif

	//Show headtitle
	g_message ("show_all 3");
	
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),hbox,doshow_vexpand,doshow_vexpand,0);

	
  	headtitle = gtk_label_new_with_mnemonic("");
	gtk_misc_set_alignment(GTK_MISC(headtitle),0,0);
	
	
	g_message ("show_all 4");
	GtkWidget *event_box1 = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (event_box1), headtitle);	
	gtk_box_pack_start(GTK_BOX(hbox),event_box1,TRUE,TRUE,0);	
	g_signal_connect (G_OBJECT (event_box1), 
                      "button_press_event",
                      G_CALLBACK (start_clock),
                      headtitle);	
	
	
	vbox_events = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),vbox_events,TRUE,TRUE,0);
    
	
	g_message ("show_all 5");
	if (((doshow_appointments==TRUE)&&(doshow_todos==TRUE))||((doshow_birthdays==TRUE)&&(doshow_todos==TRUE))) {
		button=gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(mainvbox),button,TRUE,TRUE,0);
	}
	
	vbox_todo = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),vbox_todo,TRUE,TRUE,0);

	gint count=0;
	count=show_todos(vbox_todo,count);

	g_message ("show_all 6");
	count=show_events(vbox_events,count);

	//scroll = gtk_vscrollbar_new (GTK_BOX (mainvbox)->vadj);
	//gtk_box_pack_start(GTK_BOX(hbox),scroll,TRUE,TRUE,0);
	
	if (doshow_buttons==TRUE) {
		printTime("preButtons");
	
		GtkWidget *hbox_buttons;
		hbox_buttons = gtk_hbox_new (TRUE, 1);
		gtk_box_pack_start(GTK_BOX(mainvbox),hbox_buttons,TRUE,TRUE,0);

		button = gtk_button_new_with_label(_("Calendar"));  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( calendar_gpestart ), NULL );

		button = gtk_button_new_with_label(_("Todo"));  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );

		button = gtk_button_new_with_label(_("Contacts"));  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( contacts_gpestart ), NULL );
	
		/*button = gtk_button_new_with_label("settings");  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( on_menuitem_settings ), NULL );*/
		printTime("postButtons");
	
	}

	gtk_widget_show_all(GTK_WIDGET(mainvbox));
	g_message ("show_all 7");
}


gint update_clock(gpointer data) {
  char timestring[40];
        struct tm tm;
	if (mainwidget==NULL) { return FALSE;}  //already destroyed itself?

	time_t start = time (NULL);
	//if ((lastTimeUpdate+3)>start) return TRUE;
	//lastTimeUpdate=start;
		
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&start);	
	
	//Check if day has changed
	strftime (timestring, sizeof(timestring), "%m%d", &tm);	
	if (strncmp(timestring,lastGPEDBupdate,4)!=0) {
		strftime (lastGPEDBupdate, sizeof(lastGPEDBupdate), "%m%d", &tm);	
		printTime("new Day");
		refresh_now = TRUE;
	}

	if (refresh_now==TRUE) {
	  refresh_now=FALSE;
	  show_all();
	}
	
	strftime (timestring, sizeof(timestring), "<b>%a, %d. %b. %H:%M</b>", &tm);	
	if (strcmp(gtk_label_get_label(GTK_LABEL(headtitle)),timestring)!=0) {
		gtk_label_set_markup(GTK_LABEL(headtitle),timestring);
		gtk_widget_show(GTK_WIDGET(headtitle));
	} 
	//g_message("Updating clock");
	//printTime("p3");
	return TRUE;
}


void save_prefs() {
	GKeyFile *keyfile;
	//GKeyFileFlags flags;
	//GError *error = NULL;
  
  g_message("%s",__func__);
	/* Create a new GKeyFile object and a bitwise list of flags. */
	keyfile = g_key_file_new ();

	/*if (!g_key_file_load_from_file (keyfile, label->str , flags, &error)) {
		g_warning (error->message);
		return ;
	}*/
	g_message ("save_prefs 2");  
 
	/* Read in data from the key file from the group "username". */
	g_key_file_set_boolean(keyfile, "options","show_birthdays", show_birthdays_pref);
	g_key_file_set_boolean(keyfile, "options","show_appointments", doshow_appointments);
	g_key_file_set_boolean(keyfile, "options","show_todos", doshow_todos);
	g_key_file_set_boolean(keyfile, "options","show_alltodos", doshow_alltodos);
	g_key_file_set_boolean(keyfile, "options","show_buttons", doshow_buttons);
	//g_key_file_set_boolean(keyfile, "options","show_autorefresh", doshow_autorefresh);
	g_key_file_set_boolean(keyfile, "options","show_extended", doshow_extended);
	g_key_file_set_integer(keyfile, "options", "show_countitems", doshow_countitems);

	g_message ("save_prefs 3");

	gsize length = 0;
	gchar* conf=g_key_file_to_data (keyfile,&length,NULL);

	GString *label = g_string_new( g_get_home_dir() );
	g_string_append(label,"/.gpesummary");

	g_message ("save_prefs 4");
	g_message (label->str);
	g_message (conf);

	if (g_file_set_contents (label->str,conf,length,NULL)==TRUE) { 
		g_message("saved");
	} else {
		g_warning("NOT saved");
	}

	g_key_file_free (keyfile);
	g_string_free(label,TRUE);
	g_free(conf);

	g_message ("save_prefs 5");

}

static gboolean options_clicked( GtkWidget      *button, 
			  GdkEventButton *event,
			  gpointer user_data ) {

  g_message("%s",__func__);
	GString *label = g_string_new( gtk_widget_get_name(button) );
	//g_message(label->str);
	if (strcmp(label->str,"birthdays")==0) { 
		g_message("doshow_birthdays");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { show_birthdays_pref=FALSE; } else { show_birthdays_pref=TRUE;}
		doshow_birthdays = show_birthdays_pref;
	}
	if (strcmp(label->str,"appointments")==0) { 
		g_message("appointments");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_appointments=FALSE; } else { doshow_appointments=TRUE;}
	}
	if (strcmp(label->str,"todos")==0) { 
		g_message("todos");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_todos=FALSE; } else { doshow_todos=TRUE;}
	}
	if (strcmp(label->str,"alltodos")==0) { 
		g_message("alltodos");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_alltodos=FALSE; } else { doshow_alltodos=TRUE;}
	}

	if (strcmp(label->str,"buttons")==0) { 
		g_message("buttons");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_buttons=FALSE; } else { doshow_buttons=TRUE;}
	}

	/*if (strcmp(label->str,"refresh")==0) { 
		g_message("refresh");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_autorefresh=FALSE; } else { doshow_autorefresh=TRUE;}
	}*/	

	if (strcmp(label->str,"extended")==0) { 
		g_message("extended");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_extended=FALSE; } else { doshow_extended=TRUE;}
	}	
	
	save_prefs();
	
	g_string_free(label,TRUE);
	//show_all();
	return FALSE;
}

static void options_showcountchanged(GtkComboBox *widget,gpointer user_data) {
	gint i=gtk_combo_box_get_active(widget);
	doshow_countitems=(char)((i+1)*2);
	save_prefs();
	//show_all();	
}

static void on_menuitem_settings(GtkWidget *widget, gpointer user_data)
{
  g_message("%s",__func__);
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWindow *window = GTK_WINDOW(user_data);
   
	/* Create the widgets */
   
	dialog = gtk_dialog_new_with_buttons (_("GPE Summary Options"),
					      window,
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_NONE,
					      NULL);
	//label = gtk_label_new ("What should be showed:");
   
	/* Ensure that the dialog box is destroyed when the user responds. */
   
	g_signal_connect_swapped (dialog,"response",G_CALLBACK (gtk_widget_destroy),dialog);

	/* Add the label, and show everything we've added to the dialog. */

	//gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),label);
	button = gtk_label_new(_("View will be changed after clicking OK, please be patient."));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	
	button = gtk_check_button_new_with_label(_("Show birthdays (long loading)"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "birthdays");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),show_birthdays_pref);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label(_("Show appointments"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "appointments");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_appointments);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label(_("Show todos"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "todos");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_todos);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );

	button = gtk_check_button_new_with_label(_("Show all (unfinished) todos"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "alltodos");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_alltodos);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label(_("Show start buttons"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "buttons");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_buttons);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
// 	button = gtk_check_button_new_with_label(_("Autorefresh data (long loading)"));
// 	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
// 	gtk_widget_set_name(button, "refresh");	
// 	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_autorefresh);
// 	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );	

	button = gtk_check_button_new_with_label(_("Extended view (needs more space)"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "extended");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_extended);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );	
	
	button = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~2 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~4 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~6 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~8 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~10 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~12 appointments/todos"));
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),_("Show ~14 appointments/todos"));
	gtk_combo_box_set_active(GTK_COMBO_BOX(button),((doshow_countitems+1)/2)-1);
	gtk_widget_set_name(button, "buttons");	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	g_signal_connect( GTK_OBJECT( button ), "changed",   GTK_SIGNAL_FUNC( options_showcountchanged ), NULL );
	
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_widget_show_all (GTK_WIDGET(dialog));
	gtk_dialog_run(GTK_DIALOG(dialog));
	g_message("widget showed");
	gtk_widget_destroy(dialog);

	while (gtk_events_pending()) { gtk_main_iteration(); }
	refresh_now = TRUE; // Tells update_clock to also do show_all
	update_clock(NULL);
	//show_all();
	
	
	//g_free(dialog);
}

static void calendar_changed (GnomeVFSMonitorHandle *handle,
			      const gchar *monitor_uri,
			      const gchar *info_uri,
			      GnomeVFSMonitorEventType event_type,
			      gpointer user_data)
{
  g_message("%s",__func__);
  refresh_now = TRUE; // Tells update_clock to also do show_all
  update_clock(NULL);
}

static void todo_changed (GnomeVFSMonitorHandle *handle,
			      const gchar *monitor_uri,
			      const gchar *info_uri,
			      GnomeVFSMonitorEventType event_type,
			      gpointer user_data)
{
  g_message("%s",__func__);
  refresh_now = TRUE; // Tells update_clock to also do show_all
  update_clock(NULL);
}


/*
gboolean focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
  //g_message("Focus in event");
	if (last_gui_update+3 < time(NULL)) {
		show_all();
		g_message("gui updated");
	}
	return TRUE;
}

int main (int argc, char *argv[])
{
    HildonProgram *program;
    HildonWindow *main_window;

    osso = osso_initialize ("gpesummary", "0.6", FALSE, NULL);
    if (! osso)
	    return -1;


    gtk_init(&argc, &argv);
    
    //setlocale(LC_ALL, "");
    //bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
    //bind_textdomain_codeset(PACKAGE, "UTF-8");
    //textdomain(PACKAGE);

    program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("GPE Summary");

    main_window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program, main_window);
    
#if MAEMO_VERSION_MAJOR >= 5
    widget_window = main_window;
#else
    widget_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add( GTK_CONTAINER( main_window ), widget_window );	
#endif

    show_all();
    update_clock(NULL);
    
    gtk_widget_show_all(GTK_WIDGET(window));

    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(GTK_WIDGET(window), "focus-in-event", G_CALLBACK(focus_in), NULL);

    save_prefs();
    
    g_timeout_add(2000, update_clock, NULL);

    gtk_main();


    return 0;
}*/

static guint log_handler_id = 0;
#define NOLOG_HANDLER "/tmp/gpesummary.log"
#ifdef LOG_HANDLER
static void log_handler (const char *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
  static FILE *log = NULL;
  time_t now = time(NULL);
  struct tm tm;
  char buf[200];

  if (!log) log = fopen(LOG_HANDLER,"a");

  tm=*localtime(&now);
  strftime (buf, sizeof (buf), "%c", &tm);

  fprintf(log, "[%s] %s:%x:%s\n", buf, log_domain, log_level, message);
  fflush(log);
}
#endif

#if MAEMO_VERSION_MAJOR < 5
//Home Applet stuff

static void add_home_applet_timer (void);

/* Do updates for this minute */
static gboolean home_applet_timer (gpointer data)
{
  current_timer = 0; /* We will not be repeating this timer */

  update_clock(NULL);

  /* Set new timer for next minute */
  add_home_applet_timer();

  /* Do not repeat this timer */
  return FALSE;
}

/* Add a time entry for the transition to the next minute,
   even if we are running late */
static void add_home_applet_timer (void)
{
  GTimeVal now;
  int delay;

  g_get_current_time(&now);

  delay = 60 - (now.tv_sec % 60);

  current_timer = g_timeout_add (delay * 1000, home_applet_timer, NULL);
}

void *
		hildon_home_applet_lib_initialize (void *state_data,
		int *state_size,
		GtkWidget **widget)
{
#ifdef LOG_HANDLER
  log_handler_id = g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION, log_handler, NULL);
#endif

  g_message("%s",__func__);

  reset_globals();

	g_type_init();
 	//setlocale(LC_ALL, "");
 	//bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
 	//bind_textdomain_codeset(PACKAGE, "UTF-8");
 	//textdomain(PACKAGE);
	
	calendar_file = CALENDAR_FILE();
	gnome_vfs_monitor_add (&monitor_calendar, calendar_file, GNOME_VFS_MONITOR_FILE, calendar_changed, NULL);
	todo_file = TODO_FILE();
	gnome_vfs_monitor_add (&monitor_todo, todo_file, GNOME_VFS_MONITOR_FILE, todo_changed, NULL);
	
	osso = osso_initialize ("gpesummary", "0.7.2", FALSE, NULL);
    	if (!osso) {
        	g_debug ("Error initializing the osso maemo gpesummary applet");
        	return NULL;
    	}

	/* Initialize the GTK. */
	mainwidget = gtk_frame_new(NULL); //gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (mainwidget, "GPE Summary");
	gtk_widget_set_size_request (mainwidget, WIDTH, HEIGHT); 
	*widget = mainwidget;
	g_message("mainwidget created");

  
  	gtk_container_set_border_width (GTK_CONTAINER (mainwidget), 0);

  	GtkWidget* alignment = gtk_alignment_new (0.5,0.5,1.0,1.0);

  	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 15, 15, 15, 15);

	gtk_container_add (GTK_CONTAINER (mainwidget), alignment);
#if MAEMO_VERSION_MAJOR >= 5
	widget_window = alignment;
#else
	widget_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC); //GTK_POLICY_NEVER
	gtk_container_add( GTK_CONTAINER( alignment ), widget_window );	
#endif

	//show_all();
	update_clock(NULL); //->Also shows all because it runs show_all()
	gtk_widget_show_all(GTK_WIDGET(mainwidget));
	add_home_applet_timer();
	
	return (void*)osso;
}

void
		hildon_home_applet_lib_deinitialize (void *applet_data)
{
  g_message("%s",__func__);
  if (osso) {
    osso_deinitialize (osso);
    osso = NULL;
  }
	g_message("hildon_home_applet_lib_deinitialize 2");
	g_slist_free(birthdaylist);
	birthdaylist=NULL;
	g_message("hildon_home_applet_lib_deinitialize 3");
	g_message("hildon_home_applet_lib_deinitialize 4");
	//if (mainwidget) gtk_widget_destroy(mainwidget);
	g_message("hildon_home_applet_lib_deinitialize 5");
        //if (mainwidget) g_free(mainwidget);
	mainwidget=NULL;
	mainvbox = NULL;
	headtitle = NULL;
	widget_window = NULL;
	if (current_timer) g_source_destroy(g_main_context_find_source_by_id(NULL,current_timer));
	current_timer = 0;
	g_message("hildon_home_applet_lib_deinitialize 6");
	if (monitor_calendar) gnome_vfs_monitor_cancel(monitor_calendar);
	monitor_calendar = NULL;
	if (monitor_todo) gnome_vfs_monitor_cancel(monitor_todo);
	monitor_todo = NULL;

	/*if (app) {
		g_free (app);
		app = NULL;
	}*/ //FixME

	if (log_handler_id) g_log_remove_handler(G_LOG_DOMAIN, log_handler_id);
}

int
		hildon_home_applet_lib_get_requested_width (void *data)
{
	return WIDTH;
}

int
		hildon_home_applet_lib_save_state (void *applet_data,
		void **state_data, int *state_size)
{
    /* we don't want to save state in a simple weather applet */
    
    (*state_data) = NULL;
    
    if (state_size) {
        (*state_size) = 0; 
    } else {
        g_debug ("State_size pointer was not pointing to right place");
    }
    
    return 1;
}

void
		hildon_home_applet_lib_background (void *applet_data)
{
	return;
}

void
		hildon_home_applet_lib_foreground (void *applet_data)
{
	//if (doshow_autorefresh==TRUE) refresh_clicked(NULL,NULL,NULL);
	return;
}

GtkWidget *
		hildon_home_applet_lib_settings (void *data, GtkWindow *parent)
{
	GtkWidget *settings;

	settings = gtk_menu_item_new_with_label(("GPE Summary"));
	g_signal_connect (settings, "activate",
			  G_CALLBACK (on_menuitem_settings), parent);

	return settings;

}
#endif /* MAEMO_VERSION_MAJOR < 5 */

#if MAEMO_VERSION_MAJOR >= 5
// Use HDHomePluginItem

#ifndef GPE_SUMMARY_PLUGIN_H
#define GPE_SUMMARY_PLUGIN_H

#include <glib-object.h>

#include <libhildondesktop/libhildondesktop.h>

G_BEGIN_DECLS

typedef struct _GpeSummaryPlugin GpeSummaryPlugin;
typedef struct _GpeSummaryPluginClass GpeSummaryPluginClass;

#define GPE_SUMMARY_TYPE_PLUGIN   (gpe_summary_plugin_get_type ())

#define GPE_SUMMARY_PLUGIN(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                        GPE_SUMMARY_TYPE_PLUGIN, GpeSummaryPlugin))

#define GPE_SUMMARY_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                        GPE_SUMMARY_TYPE_PLUGIN,  GpeSummaryPluginClass))

#define GPE_SUMMARY_IS_PLUGIN(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                        GPE_SUMMARY_TYPE_PLUGIN))
 
#define GPE_SUMMARY_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                        GPE_SUMMARY_TYPE_PLUGIN))

#define GPE_SUMMARY_PLUGIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                        GPE_SUMMARY_TYPE_PLUGIN,  GpeSummaryPluginClass))
 
struct _GpeSummaryPlugin
{
    HDHomePluginItem hitem;
};
 
struct _GpeSummaryPluginClass
{
    HDHomePluginItemClass parent_class;
};
 
GType gpe_summary_plugin_get_type(void);

G_END_DECLS

#endif

HD_DEFINE_PLUGIN_MODULE (GpeSummaryPlugin, gpe_summary_plugin,      HD_TYPE_HOME_PLUGIN_ITEM)

/* Do updates for this minute */
static gboolean home_applet_timer (gpointer data)
{
  if (is_visible) {
    update_clock(NULL);
    /* Repeat the timer */
    return TRUE;
  } else {
    /* Cancel the timer */
    g_message("%s: cancelling timer",__func__);
    current_timer = 0;
    return FALSE;
  }
}

static void restart_timer(GpeSummaryPlugin *desktop_plugin)
{
  current_timer = hd_home_plugin_item_heartbeat_signal_add (
		HD_HOME_PLUGIN_ITEM(desktop_plugin), 60, 70, 
		(GSourceFunc)home_applet_timer, NULL, NULL);
}

static void on_desktop_changed (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
  g_object_get(gobject, "is-on-current-desktop", &is_visible, NULL);
  g_message("%s: new visibility = %d",__func__,is_visible);

  if (is_visible) {
    update_clock(NULL);
    /* Restart the timer if necesary */
    if (current_timer == 0) restart_timer(GPE_SUMMARY_PLUGIN(gobject));
  }
}

static DBusConnection *dbus_system, *dbus_session;

static void
gpe_summary_plugin_init (GpeSummaryPlugin *desktop_plugin)
{
  DBusError dbus_error;

  g_message("%s",__func__);
  reset_globals();

  g_object_get(G_OBJECT(desktop_plugin), "is-on-current-desktop", &is_visible, NULL);
  g_signal_connect(G_OBJECT(desktop_plugin), "notify::is-on-current-desktop", G_CALLBACK(on_desktop_changed), NULL);

  calendar_file = CALENDAR_FILE();
  gnome_vfs_monitor_add (&monitor_calendar, calendar_file, GNOME_VFS_MONITOR_FILE, calendar_changed, NULL);
  todo_file = TODO_FILE();
  gnome_vfs_monitor_add (&monitor_todo, todo_file, GNOME_VFS_MONITOR_FILE, todo_changed, NULL);

  const gchar *file = hd_home_plugin_item_get_dl_filename(HD_HOME_PLUGIN_ITEM(desktop_plugin));
  g_message("plugin loaded from %s", file);

  /* Initialise OSSO */
  g_message("Initialising DBUS and OSSO");
  dbus_error_init(&dbus_error);

  if (!dbus_system) {
    dbus_system = hd_home_plugin_item_get_dbus_connection
      (HD_HOME_PLUGIN_ITEM(desktop_plugin), DBUS_BUS_SYSTEM, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) g_warning("DBus System Connection error %s: %s", dbus_error.name, dbus_error.message);
    dbus_error_free(&dbus_error);
  }

  if (!dbus_session) {
    dbus_session = hd_home_plugin_item_get_dbus_connection
      (HD_HOME_PLUGIN_ITEM(desktop_plugin), DBUS_BUS_SESSION, &dbus_error);
    if (dbus_error_is_set(&dbus_error)) g_warning("DBus System Connection error %s: %s", dbus_error.name, dbus_error.message);
    dbus_error_free(&dbus_error);
  }

  if (!osso) {
    osso = osso_initialize_with_connections ("gpesummary", "0.7.2", dbus_system, dbus_session);
    if (!osso) g_warning("Error initializing the osso context for gpesummary applet");
  }

  /* Create the GPE directory if it does not already exist */
  gchar *gpe_dir = GPE_DIR();
  if (mkdir(gpe_dir, 0777) == 0) g_message("GPE directory %s created", gpe_dir);
  g_free(gpe_dir);

  g_message("Initialising GPE Summary widget");
	mainwidget = gtk_frame_new(NULL);
	gtk_widget_set_name (mainwidget, "GPE Summary");
	gtk_widget_set_size_request (mainwidget, WIDTH, HEIGHT); 
	g_message("mainwidget created");
	
  	gtk_container_set_border_width (GTK_CONTAINER (mainwidget), 0);

  	GtkWidget* alignment = gtk_alignment_new (0.5,0.5,1.0,1.0);

  	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 15, 15, 15, 15);

	gtk_container_add (GTK_CONTAINER (mainwidget), alignment);
#if MAEMO_VERSION_MAJOR >= 5
	widget_window = alignment;
#else
	widget_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC); //GTK_POLICY_NEVER
	gtk_container_add( GTK_CONTAINER( alignment ), widget_window );	
#endif

	g_signal_connect (desktop_plugin, "show-settings",
			  G_CALLBACK (on_menuitem_settings), NULL);
	hd_home_plugin_item_set_settings(HD_HOME_PLUGIN_ITEM(desktop_plugin), TRUE);

	update_clock(NULL); //->Also shows all because it runs show_all()
	gtk_widget_show_all(GTK_WIDGET(mainwidget));

	restart_timer(desktop_plugin);
	if (!current_timer)
	  {
	    g_warning("hd_home_plugin_item_heartbeat_signal_add failed");
	  }

	gtk_container_add (GTK_CONTAINER (desktop_plugin), mainwidget);
} 

static void
gpe_summary_plugin_class_init (GpeSummaryPluginClass *class)
{
#ifdef LOG_HANDLER
  log_handler_id = g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL
                     | G_LOG_FLAG_RECURSION, log_handler, NULL);
  g_message("%s",__func__);
#endif
} 

static void
gpe_summary_plugin_class_finalize (GpeSummaryPluginClass *class)
{
  g_message("%s",__func__);

  if (osso) osso_deinitialize (osso);
  osso = NULL;
  if (dbus_system) {
    dbus_connection_close(dbus_system);
    dbus_connection_unref(dbus_system);
    dbus_system = NULL;
  }
  g_message("hildon_home_applet_lib_deinitialize 2");
  if (dbus_session) {
    dbus_connection_close(dbus_session);
    dbus_connection_unref(dbus_session);
    dbus_session = NULL;
  }

  g_slist_free(birthdaylist);
  birthdaylist=NULL;
  g_message("hildon_home_applet_lib_deinitialize 3");
  g_message("hildon_home_applet_lib_deinitialize 4");
  mainwidget = NULL;
  mainvbox = NULL;
  headtitle = NULL;
  widget_window = NULL;
  if (current_timer) g_source_destroy(g_main_context_find_source_by_id(NULL,current_timer));
  else g_warning("No current timer running");
  current_timer = 0;
  g_message("hildon_home_applet_lib_deinitialize 5");
  contacts_db_close();
  g_message("hildon_home_applet_lib_deinitialize 5a");
  if (event_db) {
    g_object_unref(event_db);
    event_db = NULL;
  }
  g_message("hildon_home_applet_lib_deinitialize 5b");
  todo_db_stop ();
  g_message("hildon_home_applet_lib_deinitialize 6");
  if (monitor_calendar) gnome_vfs_monitor_cancel(monitor_calendar);
  monitor_calendar = NULL;
  if (calendar_file) g_free(calendar_file);
  calendar_file = NULL;
  if (monitor_todo) gnome_vfs_monitor_cancel(monitor_todo);
  monitor_todo = NULL;
  if (todo_file) g_free(todo_file);
  todo_file = NULL;
  g_message("hildon_home_applet_lib_deinitialize 7");


  if (log_handler_id) g_log_remove_handler(G_LOG_DOMAIN, log_handler_id);
} 

/* Prevent unloading */
/* Note: because libeventdb registers GTypes, which contain pointers to functions in
   the libeventdb code but which cannot be unregistered or re-registered when we are 
   reloaded, we need to prevent hildon-home from unloading our code.
   This does not stop the widget from being removed from the home screen (or added back
   later) but does mean that if the plugin code is replaced the new code will not be 
   used until the next reboot (actually the next time hildon-home is restarted) even
   if the user removes and re-adds the widget. */
/* See GLib documentation on dynamic loading of modules for how the following code
   works */
/* Note: of course, this means the plugin code above has to handle the plugin being initialised 
   and finalised several times without the code being reloaded */
const gchar *
g_module_check_init (GModule *module)
{
     g_module_make_resident(module);

     return NULL;
}


#endif /* MAEMO_VERSION_MAJOR >= 5 */
