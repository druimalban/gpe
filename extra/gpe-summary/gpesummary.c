/*
 * Copyright (C) 2007 Christoph Würstle <n800@axique.de>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

//
//gcc `pkg-config --cflags --libs gtk+-2.0 hildon-libs dbus-1 libosso libeventdb libtododb libcontactsdb` main.c -o main
//


#define IS_HILDON 1

#include <glib.h>
#include <stdlib.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

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

#ifdef IS_HILDON
/* Hildon includes */

#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#include <libosso.h>
#include <hildon-home-plugin/hildon-home-plugin-interface.h>
#include <hildon-widgets/hildon-note.h>

#include <config.h>

#define APPLICATION_DBUS_SERVICE "gpesummary"
#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"
#endif

#define CALENDAR_FILE_ "/.gpe/calendar"
#define CALENDAR_FILE() \
  g_strdup_printf ("%s" CALENDAR_FILE_, g_get_home_dir ())
  
#define _(String) dgettext (PACKAGE,String)

#define WIDTH 300
#define HEIGHT 400

GtkWidget *button =NULL;
GtkWidget *mainvbox =NULL;
GtkWidget *prefsvbox =NULL;
GtkWidget *mainwidget =NULL;
GtkWidget *headtitle = NULL;
GtkWidget *settingswidget = NULL;
GtkWindow *settingswindow = NULL;
HildonWindow *window;
GtkWidget *scrolled_window;
time_t last_gui_update = 0;
char timestring[40];

gboolean doshow_birthdays=TRUE;
gboolean doshow_appointments=TRUE;
gboolean doshow_todos=TRUE;
gboolean doshow_buttons=FALSE;
gint doshow_countitems=8;
gboolean doshow_autorefresh=FALSE;
gboolean doshow_extended=TRUE; //waste space
gboolean doshow_vexpand=FALSE; //no option yet!


gboolean refresh_now=FALSE; //User asked for refresh update_clock manages this

static osso_context_t *osso;

GSList *birthdaylist =NULL;
gchar lastGPEDBupdate[6] = "xxxx";
int todocount;
struct tm tm;

void printTime(gchar * comment) {
     struct timeval tv;
     struct timezone tz;
     gettimeofday(&tv, &tz);
     tm=*localtime(&tv.tv_sec);
     printf("%s %d:%d:%d %d \n",comment, tm.tm_hour, tm.tm_min,
              tm.tm_sec, tv.tv_usec);
}

static void async_cb(const gchar *interface, const gchar *method,
                     osso_rpc_t *retval, gpointer data)
{
        printf("method '%s' returned\n", method);
}

void todo_gpestart(GtkButton *button, gpointer user_data) {
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
                printf("ERROR!\n");
        }
}

static void todo_clicked( GtkWidget      *button, 
					GdkEventButton *event,
					gpointer user_data ) {
	//g_message(gtk_widget_get_name(button));	
	todo_db_start ();
	
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
		g_message("adding no todos");
		GtkWidget *eventbox;
		eventbox = gtk_event_box_new();
		gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
		button = gtk_label_new_with_mnemonic(_("(no todos)"));
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );
		gtk_widget_show_all(GTK_WIDGET(vbox));
	}	
		
	g_slist_free(iter);
}


void calendar_gpestart(GtkButton *button, gpointer user_data) {
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
                printf("ERROR!\n");
        }	
}

void contacts_gpestart(GtkButton *button, gpointer user_data) {
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
                printf("ERROR!\n");
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
                printf("ERROR!\n");
                return 1;
        }
	return FALSE;
}


static gboolean todo_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	todo_gpestart(GTK_BUTTON(button), user_data);
	return FALSE;
}

static gboolean events_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	calendar_gpestart(GTK_BUTTON(button), user_data);
	return FALSE;
}

static gboolean contacts_startclicked(GtkWidget *button, GdkEventButton *event, gpointer user_data ) {
	contacts_gpestart(GTK_BUTTON(button), user_data);
	return FALSE;
}


gint show_todos(GtkWidget *vbox, gint count) {
	if (doshow_todos==FALSE) return count;
	time_t tmp = time (NULL);
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&tmp);
	time_t todaystop = time (NULL)+(23-tm.tm_hour)*3600+(60-tm.tm_min)*60;
	time_t todaystart = time (NULL)-tm.tm_hour*3600-tm.tm_min*60-tm.tm_sec;
	todocount=0;
	
	todo_db_start ();
	

	GSList *iter;

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		if (((struct todo_item *)iter->data)->state == COMPLETED)
			continue;
		if (((struct todo_item *)iter->data)->time > todaystop)
			continue;
		if (((struct todo_item *)iter->data)->time == 0)
			continue;
		todocount+=1;
		
		GString *label = g_string_new( (((struct todo_item *)iter->data)->summary) );
		if (((struct todo_item *)iter->data)->time < todaystart) {
			g_string_append(label," (!)");
		}

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
		gtk_box_pack_start(GTK_BOX(hbox_todo),eventbox,doshow_vexpand,doshow_vexpand,0);
		
		button = gtk_label_new(label->str);
		gtk_container_add(GTK_CONTAINER(eventbox),button);//,TRUE,TRUE,0);
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_misc_set_alignment(GTK_MISC(button),0,0.5);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_startclicked ), NULL );	
	}
	
	if (todocount==0) {
		//g_warning("todocount 0");
		GtkWidget *eventbox;
		eventbox = gtk_event_box_new();
		gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
		button = gtk_label_new_with_mnemonic(_("(no todos)"));
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );

		
		todocount+=1;
	}	

	todo_db_stop ();
	
	g_slist_free(iter);
	gtk_widget_show_all(GTK_WIDGET(vbox));
	g_warning("todosjow finished");
	return todocount+count;
}

void show_title(GtkWidget *vbox,gchar *title) {
	if (title!=NULL) {
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
void prepare_birthdays() {
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

	//g_warning( buf2 );
	GSList *iter;

	struct contacts_person *p;
	
	for (iter = contacts_db_get_entries(); iter; iter = g_slist_next(iter)) {
		struct contacts_tag_value *ctv;
		p = contacts_db_get_by_uid(((struct contacts_person *)iter->data)->id);
		if (p) {
		  ctv=contacts_db_find_tag(p,"BIRTHDAY");
		} else ctv=NULL;

		if (ctv) {
			gboolean found=FALSE;
			if (strncmp((ctv->value)+4, day1,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day2,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day3,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day4,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day5,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day6,4)==0) found=TRUE;
			if (strncmp((ctv->value)+4, day7,4)==0) found=TRUE;
			
			if(found==TRUE) {
				int i =((struct contacts_person *)iter->data)->id;
				birthdaylist = g_slist_append (birthdaylist, GINT_TO_POINTER (i));
			}
			
		}
	}
	g_slist_free(iter);
}

gboolean show_birthdays (GtkWidget *vbox,time_t start,gchar *title) {

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
	
	//g_warning("birthdaylist reading %p",&birthdaylist);
	
	for (iter = birthdaylist; iter; iter = g_slist_next(iter)) {
		guint id = (guint)iter->data;
		g_warning("birthday id %i",id);
		p = contacts_db_get_by_uid(id);
		ctv=contacts_db_find_tag(p,"BIRTHDAY");
		
		if (strncmp(buf2,(ctv->value)+4,4)==0) {
				g_warning(ctv->value);
				if (titletoshow==TRUE) show_title(vbox,title);
				titletoshow=FALSE;
				GString *label = g_string_new( _(" Birthday "));
				ctv=contacts_db_find_tag(p,"NAME");
				g_string_append(label,ctv->value);
				g_warning( label->str );

				GtkWidget *eventbox= gtk_event_box_new();
				gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);
		
				button = gtk_label_new_with_mnemonic(label->str);  
				gtk_container_add(GTK_CONTAINER(eventbox),button);
				gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
				gtk_misc_set_alignment(GTK_MISC(button),0,0);
				g_signal_connect( G_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( contacts_startclicked ), NULL );	


		}

			
	}
	
	g_slist_free(iter);
	return titletoshow;
}


gint add_events(GtkWidget *vbox,EventDB *event_db, time_t start, time_t stop, gchar *title, gboolean showtitle, gint count) {
	
	if (doshow_appointments==FALSE) return count;
	
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
		  	  strftime (buf, sizeof (buf), "%a", &tm); //%d.%m 
			}
			if ((event_get_start(ev)-start)>0) ignoreEvent=TRUE;
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
			gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);		
			button = gtk_label_new_with_mnemonic(buf);  
			gtk_container_add(GTK_CONTAINER(eventbox),button);
			gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
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
		gtk_box_pack_start(GTK_BOX(vbox),eventbox,doshow_vexpand,doshow_vexpand,0);	
		button = gtk_label_new_with_mnemonic(_(" (no appointments)"));
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_widget_set_events(eventbox,GDK_BUTTON_PRESS_MASK);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		g_signal_connect( GTK_OBJECT( eventbox ), "button_press_event",   GTK_SIGNAL_FUNC( events_startclicked ), NULL );
		count+=1;
	}
	
	g_slist_free(events);
	g_slist_free(iter);
	return count;
}


gint show_events(GtkWidget *vbox, gint count) {
	time_t start = time (NULL);
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&start);
	time_t stop = time (NULL)+(23-tm.tm_hour)*3600+(59-tm.tm_min)*60+59-tm.tm_sec;
	
	/*char buf[200];
	tm=*localtime(&stop);
	strftime (buf, sizeof (buf), "%d %H:%M %S", &tm); //%d.%m 
	g_warning("endtime");
	g_warning(buf);*/
	
	EventDB *event_db = NULL;
	char *filename = CALENDAR_FILE ();	
	if (doshow_appointments==TRUE) {
		GError **error = NULL;
		event_db = event_db_new (filename, error);
		if (! event_db)
		{
			g_critical ("Failed to open event database.");
			exit (1);
		}
	}	

	if (doshow_birthdays==TRUE) contacts_db_open(FALSE);
	prepare_birthdays();

	//Today already set as head	
	gboolean titletoshow=show_birthdays(vbox,start,NULL);
	count+=add_events(vbox,event_db,start, stop, NULL, titletoshow,0);

	//titletoshow=show_birthdays(vbox,stop+2,"Tomorrow");
	//count=add_events(vbox,event_db,stop+2,stop+3600*24-2,"Tomorrow",titletoshow, count);
		
	//g_warning("Tomorrow %i",count);
	stop=stop+2; //Make sure you are in the right day;
	while ((count<doshow_countitems)&&(stop<start+3600*24*6)) {
		char buf2[20];
		//g_warning("Tomorrow %i",count);
		memset (&tm, 0, sizeof (tm));
		memset (&buf2, 0, sizeof (buf2));
		tm=*localtime(&stop);
	
		strftime (buf2, sizeof(buf2), "<i>%A</i>", &tm);
		
		titletoshow=show_birthdays(vbox,stop+20,buf2);
		count=add_events(vbox,event_db,stop+50,stop+3600*24-50,buf2,titletoshow, count);
		stop=stop+3600*24;
	}

	if (doshow_birthdays==TRUE) contacts_db_close ();
	printTime("Events finished");
	
	return count;
	
}

void loadPrefs(){
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError **error = NULL;
        GError *error2 = NULL;

	g_warning ("load_prefs 1");
  
	/* Create a new GKeyFile object and a bitwise list of flags. */
	keyfile = g_key_file_new ();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
  
	/* Load the GKeyFile from keyfile.conf or return. */
	GString *label = g_string_new( g_get_home_dir() );
	g_string_append(label,"/.gpesummary");

	g_warning ("load_prefs 2");

	if (!g_key_file_load_from_file (keyfile, label->str , flags, &error2)) {
		g_warning ("failed to open conffile");
		g_warning (error2->message);
		g_warning ("failed to open conffile");
		return ;
	}
 
	g_warning ("load_prefs 3");

	/* Read in data from the key file from the group "username". */
	doshow_birthdays = g_key_file_get_boolean(keyfile, "options","show_birthdays", error);
	doshow_appointments = g_key_file_get_boolean(keyfile, "options","show_appointments", error);
	doshow_todos = g_key_file_get_boolean(keyfile, "options","show_todos", error);
	doshow_buttons = g_key_file_get_boolean(keyfile, "options","show_buttons", error);
	doshow_autorefresh = g_key_file_get_boolean(keyfile, "options","show_autorefresh", error);
	doshow_countitems = g_key_file_get_integer(keyfile, "options", "show_countitems", error);
	doshow_extended = g_key_file_get_boolean(keyfile, "options","show_extended", error);

	g_warning ("load_prefs 4");
	g_key_file_free (keyfile);
	g_string_free(label,TRUE);

	g_warning ("load_prefs 5");
}

static gboolean refresh_clicked( GtkWidget      *button, 
					GdkEventButton *event,
					gpointer user_data ) {
		
	//hildon_banner_show_information(mainvbox,NULL,_("Refreshing"));
	if (headtitle) {
		gtk_label_set_text(GTK_LABEL(headtitle),_("Refreshing"));
		gtk_widget_show(GTK_WIDGET(headtitle));
	}
	refresh_now=TRUE;
	return FALSE;
}


void show_all() {
	g_warning ("show_all 1");	
	last_gui_update=time(NULL);
	loadPrefs();
	g_warning ("show_all 2");
	GtkWidget *vbox_todo;
	GtkWidget *vbox_events;
	
	
	if (mainvbox!=NULL) gtk_widget_destroy(mainvbox);
	
	mainvbox = gtk_vbox_new (FALSE, 0);
	gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(scrolled_window), mainvbox );
    	
	g_warning ("show_all 3");

	//Show headtitle
	
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),hbox,doshow_vexpand,doshow_vexpand,0);

	
  	headtitle = gtk_label_new_with_mnemonic("");
	gtk_misc_set_alignment(GTK_MISC(headtitle),0,0);
	
	
	GtkWidget *event_box1 = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (event_box1), headtitle);	
	gtk_box_pack_start(GTK_BOX(hbox),event_box1,TRUE,TRUE,0);	
	g_signal_connect (G_OBJECT (event_box1), 
                      "button_press_event",
                      G_CALLBACK (start_clock),
                      headtitle);	
	
	GtkWidget *refresh_image=gtk_image_new_from_icon_name("qgn_toolb_gene_refresh",
				     GTK_ICON_SIZE_LARGE_TOOLBAR);
	
	g_warning ("show_all 4");	

	GtkWidget *event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (event_box), refresh_image);
	gtk_misc_set_alignment(GTK_MISC(refresh_image),0,0);
	g_signal_connect (G_OBJECT (event_box), 
                      "button_press_event",
                      G_CALLBACK (refresh_clicked),
                      refresh_image);
	
	gtk_box_pack_start(GTK_BOX(hbox), event_box, FALSE, FALSE, 0);	
	
	g_warning ("show_all 5");	
	//Begin rss abschreiben
/*   	GtkWidget *rss_image = g_object_new (
  
*/	
	//Ende abschreiben
	
	vbox_events = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),vbox_events,TRUE,TRUE,0);
    
	
	
	if (((doshow_appointments==TRUE)&&(doshow_todos==TRUE))||((doshow_birthdays==TRUE)&&(doshow_todos==TRUE))) {
		button=gtk_hseparator_new();
		gtk_box_pack_start(GTK_BOX(mainvbox),button,TRUE,TRUE,0);
	}
	
	vbox_todo = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start(GTK_BOX(mainvbox),vbox_todo,TRUE,TRUE,0);

	gint count=0;
	count=show_todos(vbox_todo,count);
	count=show_events(vbox_events,count);
	

	
	//scroll = gtk_vscrollbar_new (GTK_BOX (mainvbox)->vadj);
	//gtk_box_pack_start(GTK_BOX(hbox),scroll,TRUE,TRUE,0);
	
	g_warning ("show_all 6");

	if (doshow_buttons==TRUE) {
		printTime("preButtons");
	
		GtkWidget *hbox_buttons;
		hbox_buttons = gtk_hbox_new (FALSE, 1);
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
	g_warning ("show_all 7");
}


gint update_clock(gpointer data) {
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
		show_all();
	} else {
		if (refresh_now==TRUE) {
			refresh_now=FALSE;
			show_all();
		}
	}	
	refresh_now=FALSE;
	
	strftime (timestring, sizeof(timestring), "<b>%a, %d. %b. %H:%M</b>", &tm);	
	if (strcmp(gtk_label_get_label(GTK_LABEL(headtitle)),timestring)!=0) {
		gtk_label_set_markup(GTK_LABEL(headtitle),timestring);
		gtk_widget_show(GTK_WIDGET(headtitle));
	} 
	//g_warning("Updating clock");
	//printTime("p3");
	return TRUE;
}


void save_prefs() {
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
  
	g_warning ("save_prefs 1");
	/* Create a new GKeyFile object and a bitwise list of flags. */
	keyfile = g_key_file_new ();

	/*if (!g_key_file_load_from_file (keyfile, label->str , flags, &error)) {
		g_warning (error->message);
		return ;
	}*/
	g_warning ("save_prefs 2");  
 
	/* Read in data from the key file from the group "username". */
	g_key_file_set_boolean(keyfile, "options","show_birthdays", doshow_birthdays);
	g_key_file_set_boolean(keyfile, "options","show_appointments", doshow_appointments);
	g_key_file_set_boolean(keyfile, "options","show_todos", doshow_todos);
	g_key_file_set_boolean(keyfile, "options","show_buttons", doshow_buttons);
	g_key_file_set_boolean(keyfile, "options","show_autorefresh", doshow_autorefresh);
	g_key_file_set_boolean(keyfile, "options","show_extended", doshow_extended);
	g_key_file_set_integer(keyfile, "options", "show_countitems", doshow_countitems);

	g_warning ("save_prefs 3");

	gsize length;
	gchar* conf=g_key_file_to_data (keyfile,&length,error);

	GString *label = g_string_new( g_get_home_dir() );
	g_string_append(label,"/.gpesummary");

	g_warning ("save_prefs 4");
	g_warning (label->str);
	g_warning (conf);

	if (g_file_set_contents (label->str,conf,length,NULL)==TRUE) { 
		g_warning("saved");
	} else {
		g_warning("NOT saved");
	}

	g_key_file_free (keyfile);
	g_string_free(label,TRUE);
	g_free(conf);

	g_warning ("save_prefs 5");

}

static gboolean options_clicked( GtkWidget      *button, 
			  GdkEventButton *event,
			  gpointer user_data ) {

	g_warning("options_clicked");
	GString *label = g_string_new( gtk_widget_get_name(button) );
	//g_warning(label->str);
	if (strcmp(label->str,"birthdays")==0) { 
		g_warning("doshow_birthdays");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_birthdays=FALSE; } else { doshow_birthdays=TRUE;}
	}
	if (strcmp(label->str,"appointments")==0) { 
		g_warning("appointments");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_appointments=FALSE; } else { doshow_appointments=TRUE;}
	}
	if (strcmp(label->str,"todos")==0) { 
		g_warning("todos");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_todos=FALSE; } else { doshow_todos=TRUE;}
	}
	if (strcmp(label->str,"buttons")==0) { 
		g_warning("buttons");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_buttons=FALSE; } else { doshow_buttons=TRUE;}
	}

	if (strcmp(label->str,"refresh")==0) { 
		g_warning("refresh");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_autorefresh=FALSE; } else { doshow_autorefresh=TRUE;}
	}	

	if (strcmp(label->str,"extended")==0) { 
		g_warning("extended");
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))==FALSE) { doshow_extended=FALSE; } else { doshow_extended=TRUE;}
	}	
	
	save_prefs();
	
	g_string_free(label,TRUE);
	show_all();
	return FALSE;
}

static void options_showcountchanged(GtkComboBox *widget,gpointer user_data) {
	gint i=gtk_combo_box_get_active(widget);
	doshow_countitems=(char)((i+1)*2);
	save_prefs();
	show_all();	
}

static void on_menuitem_settings(GtkWidget *widget, gpointer user_data)
{
    /*if (!window && user_data)
	window = gtk_widget_get_ancestor(GTK_WIDGET(user_data), GTK_TYPE_WINDOW);
    
	if (window && GTK_IS_WINDOW(window))*/
	//execute_rss_settings(rss_appl_inf->osso, NULL, TRUE);
	g_warning("on_menuitem_settings");
	//settingswidget=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//gtk_widget_show_all(GTK_WIDGET(settingswidget));

	GtkWidget *dialog;
   
	/* Create the widgets */
   
	dialog = gtk_dialog_new_with_buttons (_("GPE Summary Options"),
					      settingswindow,
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_OK,
					      GTK_RESPONSE_NONE,
					      NULL);
	//label = gtk_label_new ("What should be showed:");
   
	/* Ensure that the dialog box is destroyed when the user responds. */
   
	g_signal_connect_swapped (dialog,"response",G_CALLBACK (gtk_widget_destroy),dialog);

	/* Add the label, and show everything we've added to the dialog. */

	//gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),label);
	button = gtk_label_new(_("After clicking view will be changed, so be patient."));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	
	button = gtk_check_button_new_with_label(_("Show birthdays (long loading)"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "birthdays");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_birthdays);
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
	
	button = gtk_check_button_new_with_label(_("Show start buttons"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "buttons");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_buttons);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label(_("Autorefresh data (long loading)"));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "refresh");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_autorefresh);
	g_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );	

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
	gtk_combo_box_set_active(GTK_COMBO_BOX(button),round((doshow_countitems/2)-1));
	gtk_widget_set_name(button, "buttons");	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	g_signal_connect( GTK_OBJECT( button ), "changed",   GTK_SIGNAL_FUNC( options_showcountchanged ), NULL );
	
	gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);
	gtk_widget_show_all (GTK_WIDGET(dialog));
	
	g_warning("widget showed");
	
	
	
	//gtk_widget_destroy(dialog);
	//g_free(dialog);
}



gboolean focus_in(GtkWidget *widget, GdkEventFocus *event, gpointer user_data) {
  //g_warning("Focus in event");
	if (last_gui_update+3 < time(NULL)) {
		show_all();
		g_warning("gui updated");
	}
	return TRUE;
}

int main (int argc, char *argv[])
{
    HildonProgram *program;

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

    window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program, window);
    
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add( GTK_CONTAINER( window ), scrolled_window );	

    show_all();
    update_clock(NULL);
    
    gtk_widget_show_all(GTK_WIDGET(window));

    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(GTK_WIDGET(window), "focus-in-event", G_CALLBACK(focus_in), NULL);

    save_prefs();
    
    g_timeout_add(2000, update_clock, NULL);

    gtk_main();


    return 0;
}





//Home Applet stuff
void *
		hildon_home_applet_lib_initialize (void *state_data,
		int *state_size,
		GtkWidget **widget)
{

	g_type_init();
 	//setlocale(LC_ALL, "");
 	//bindtextdomain(PACKAGE, PACKAGE_LOCALE_DIR);
 	//bind_textdomain_codeset(PACKAGE, "UTF-8");
 	//textdomain(PACKAGE);
	
	
	osso = osso_initialize ("gpesummary", "0.7.0", FALSE, NULL);
    	if (!osso) {
        	g_debug ("Error initializing the osso maemo gpesummary applet");
        	return NULL;
    	}

	/* Initialize the GTK. */
	mainwidget = gtk_frame_new(NULL); //gtk_vbox_new (FALSE, 0);
	gtk_widget_set_name (mainwidget, "osso-speeddial");
	gtk_widget_set_size_request (mainwidget, WIDTH, HEIGHT); 
	*widget = mainwidget;
	g_warning("mainwidget created");

  
  	gtk_container_set_border_width (GTK_CONTAINER (mainwidget), 0);

  	GtkWidget* alignment = gtk_alignment_new (0.5,
                                 0.5,
                                 1.0,
                                 1.0);

  	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 15, 15, 15, 15);

	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC); //GTK_POLICY_NEVER
	gtk_container_add (GTK_CONTAINER (mainwidget), alignment);
	gtk_container_add( GTK_CONTAINER( alignment ), scrolled_window );	

	show_all();
	update_clock(NULL); //->Also shows all because it runs show_all()
	gtk_widget_show_all(GTK_WIDGET(mainwidget));
	g_timeout_add(5000, update_clock, NULL);
	
	return (void*)osso;
}

void
		hildon_home_applet_lib_deinitialize (void *applet_data)
{
	if (osso)
		osso_deinitialize (osso);
	g_slist_free(birthdaylist);
	birthdaylist=NULL;
	if (prefsvbox) gtk_widget_destroy(prefsvbox);
	if (mainwidget) gtk_widget_destroy(mainwidget);
        //if (mainwidget) g_free(mainwidget);
	mainwidget=NULL;
	/*if (app) {
		g_free (app);
		app = NULL;
	}*/ //FixME
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
	if (doshow_autorefresh==TRUE) refresh_clicked(NULL,NULL,NULL);
	return;
}

/*static gint prefs_ok( GtkWidget      *button, 
			  GdkEventButton *event,
			  gpointer user_data ) {
				  
	gtk_widget_hide(prefsvbox);
	if (prefsvbox) gtk_widget_destroy(prefsvbox);
	
}*/

GtkWidget *
		hildon_home_applet_lib_settings (void *data, GtkWindow *parent)
{

	settingswindow = parent;
    
	GtkWidget *settings;

	settings = gtk_menu_item_new_with_label(("GPE Summary"));
	g_signal_connect (settings, "activate",
			  G_CALLBACK (on_menuitem_settings), NULL);

	return settings;
	return NULL;

}