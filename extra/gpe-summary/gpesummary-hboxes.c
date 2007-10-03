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

#ifdef IS_HILDON
/* Hildon includes */

#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-window.h>
#include <libosso.h>
#include <hildon-home-plugin/hildon-home-plugin-interface.h>

#define APPLICATION_DBUS_SERVICE "gpesummary"
#define ICON_PATH "/usr/share/icons/hicolor/26x26/hildon"
#endif

#define CALENDAR_FILE_ "/.gpe/calendar"
#define CALENDAR_FILE() \
  g_strdup_printf ("%s" CALENDAR_FILE_, g_get_home_dir ())

GtkWidget *button;
GtkWidget *mainvbox;
GtkWidget *prefsvbox;
GtkWidget *mainwidget;
GtkWidget *settingswidget = NULL;
GtkWindow *settingswindow = NULL;
HildonWindow *window;
GtkWidget *scrolled_window;
time_t last_gui_update = 0;

gboolean doshow_birthdays=TRUE;
gboolean doshow_appointments=TRUE;
gboolean doshow_todos=TRUE;
gboolean doshow_buttons=TRUE;
gint doshow_countitems=6;

#define WIDTH 300
#define HEIGHT 400

static osso_context_t *osso;

GSList *birthdaylist =NULL;
gchar *lastbirthdaylistupdate = "xxxx";
int todocount;
#define conffilename "/home/user/apps/gpesummary.conf"

void printTime(gchar * comment) {
     struct timeval tv;
     struct timezone tz;
     struct tm *tm;
     gettimeofday(&tv, &tz);
     tm=localtime(&tv.tv_sec);
     printf("%s %d:%02d:%02d %d \n",comment, tm->tm_hour, tm->tm_min,
              tm->tm_sec, tv.tv_usec);
}

static gint todo_clicked( GtkWidget      *button, 
					GdkEventButton *event,
					gpointer user_data ) {
	//g_message(gtk_widget_get_name(button));	
	todo_db_start ();
	
	GSList *iter;

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		g_message(gtk_widget_get_name(button));
		GString *label = g_string_new( gtk_widget_get_name(button) );
		//g_string_append label->append(" (!)");
		if (strcmp(((struct todo_item *)iter->data)->todoid,gtk_widget_get_name(button))==0) {
			((struct todo_item *)iter->data)->state = COMPLETED;
			g_message("set to completed");
			todo_db_push_item(((struct todo_item *)iter->data));
			
			//todo_db_delete_item(((struct todo_item *)iter->data));
		}
	}
	
	todo_db_stop();
	GtkWidget *vbox = gtk_widget_get_parent(button);
	gtk_widget_destroy(button);
	todocount--;
	g_message("todocount %i",todocount);
	if (todocount==0) {
		g_message("adding no todos");
		//const gchar *labeltitle=("(no todos)");
		button = gtk_label_new_with_mnemonic("(no todos)");
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
		gtk_widget_show_all(GTK_WIDGET(vbox));
	}	
		
	g_slist_free(iter);
}

void todo_gpestart(GtkButton *button, gpointer user_data) {
	g_message("starting gpe-todo");
        system("gpe-todo &");
}

void calendar_gpestart(GtkButton *button, gpointer user_data) {
	g_message("starting gpe-calendar");
	system("gpe-calendar &");
}

void contacts_gpestart(GtkButton *button, gpointer user_data) {
	g_message("starting gpe-contacts");
	system("gpe-contacts &");
}
    
static gboolean todo_startclicked( GtkWidget      *button, 
					GdkEventButton *event,
					gpointer user_data ) {
	todo_gpestart(GTK_BUTTON(button), user_data);
	return FALSE;
}
    
gint show_todos(GtkWidget *vbox, gint count) {
	
	if (doshow_todos==FALSE) return count;
	
	time_t tmp = time (NULL);
	struct tm tm;
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&tmp);
	time_t todaystop = time (NULL);
	time_t todaystart = todaystop-tm.tm_hour*3600-tm.tm_min*60-tm.tm_sec;
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
		
		//gtk_entry_set_text(glade_xml_get_widget(xml,"entry1"),((struct todo_item *)iter->data)->summary);
		GString *label = g_string_new( (((struct todo_item *)iter->data)->summary) );
		if (((struct todo_item *)iter->data)->time < todaystart) {
			g_string_append(label," (!)");
		}

		
		GtkWidget *hbox_todo;
		GtkWidget *eventbox;
		hbox_todo = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox),hbox_todo,TRUE,TRUE,0);

		button = gtk_check_button_new_with_label(""); //label->str);  
		gtk_box_pack_start(GTK_BOX(hbox_todo),button,TRUE,TRUE,0);
		gtk_widget_set_name(button, ((struct todo_item *)iter->data)->todoid);	
		
		gtk_signal_connect( GTK_OBJECT( button ), "button-release-event",   GTK_SIGNAL_FUNC( todo_clicked ), NULL );
		
		eventbox= gtk_event_box_new();
		gtk_box_pack_start(GTK_BOX(hbox_todo),eventbox,TRUE,TRUE,0);
		
		button = gtk_label_new(label->str);
		gtk_box_pack_start(GTK_EVENT_BOX(eventbox),button,TRUE,TRUE,0);
		gtk_widget_set_events(GTK_WIDGET(eventbox),GDK_BUTTON_PRESS_MASK);
		
		gtk_container_add(GTK_CONTAINER(eventbox),button);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		
		gtk_signal_connect( GTK_OBJECT( button ), "button-release-event",   GTK_SIGNAL_FUNC( todo_startclicked ), NULL );		
		//g_warning(((struct todo_item *)iter->data)->todoid);

	}
	
	if (todocount==0) {
		//const gchar *labeltitle=();
		button = gtk_label_new_with_mnemonic("(no todos)");
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
		todocount+=1;
	}	

	todo_db_stop ();
	
	g_slist_free(iter);
	return todocount+count;
}

void show_title(GtkWidget *vbox,gchar *title) {
	button = gtk_label_new_with_mnemonic(title);
			
	gtk_misc_set_alignment(GTK_MISC(button),0,0);
	gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
}

void prepare_birthdays() {
	
	if (doshow_birthdays==FALSE) return;

	char day1[5],day2[5],day3[5],day4[5],day5[5],day6[5],day7[5];
	time_t start = time (NULL);
	struct tm tm;
	//memset (&tm, 0, sizeof (tm));
	//memset (&day1, 0, sizeof (day1));

	tm=*localtime(&start);	
	strftime (day1, sizeof(day1), "%m%d", &tm);

	//Check if day has changed
	if (strncmp(day1,lastbirthdaylistupdate,4)==0) {
			return ;
	}
	g_slist_free(birthdaylist);
	birthdaylist=NULL;
	
	lastbirthdaylistupdate=day1;
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
	/*for (iter = birthdaylist; iter; iter = g_slist_next(iter)) {
		g_warning("id list %i",iter->data);
	} */
	
	g_slist_free(iter);
}

gboolean show_birthdays (GtkWidget *vbox,time_t start,gchar *title) {

	if (doshow_birthdays==FALSE) return TRUE;
	 
	char buf2[5];
	struct tm tm;
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
				GString *label = g_string_new( " Birthday ");
				ctv=contacts_db_find_tag(p,"NAME");
				g_string_append(label,ctv->value);
				g_warning( label->str );

				button = gtk_label_new_with_mnemonic(label->str);  
				gtk_misc_set_alignment(GTK_MISC(button),0,0);
				gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
		}

			
	}
	
	g_slist_free(iter);
	return titletoshow;
}


gint add_events(GtkWidget *vbox,EventDB *event_db, time_t start, time_t stop, gchar *title, gboolean showtitle, gint count) {
	
	if (doshow_appointments==FALSE) return count;
	
	struct tm tm;
	memset (&tm, 0, sizeof (tm));
	
	
	GSList *events = event_db_list_for_period (event_db, start ,stop );
	events = g_slist_sort (events, event_compare_func);
	GSList *iter;
	for (iter = events; iter; iter = iter->next)
	{
		count+=1;
		
		if (showtitle==TRUE) {
			showtitle=FALSE;
			show_title(vbox,title);
		}
		
		Event *ev = EVENT (iter->data);
		
		char buf[200];
		time_t tt=event_get_start(ev);
		tm=*localtime(&tt);
		strftime (buf, sizeof (buf), " %H:%M", &tm); //%d.%m 
		strcat(buf,"-");
		
		char buf2[20];
		tt=event_get_start(ev)+event_get_duration(ev);
		tm=*localtime(&tt);
		strftime (buf2, sizeof (buf2), "%H:%M", &tm); //%d.%m 		
		strcat(buf,buf2);
		if (strncmp(buf,(" 00:00-00:00"),12)==0) memset (&buf, 0, sizeof (buf)); //Allday event!
		strcat(buf," ");
		  
		strcat(buf,event_get_summary(ev) );
		button = gtk_label_new_with_mnemonic(buf);
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
	}
	
	//show today for sure
	if (count==0) {
		if (showtitle==TRUE) {
			showtitle=FALSE;
			show_title(vbox,title);
		}
		
		button = gtk_label_new_with_mnemonic(" (no appointments)");
		gtk_misc_set_alignment(GTK_MISC(button),0,0);
		gtk_box_pack_start(GTK_BOX(vbox),button,TRUE,TRUE,0);
		count+=1;
	}
	
	g_slist_free(events);
	g_slist_free(iter);
	return count;
}

gint show_events(GtkWidget *vbox, gint count) {
	time_t start = time (NULL);
	struct tm tm;
	memset (&tm, 0, sizeof (tm));	
	tm=*localtime(&start);
	time_t stop = time (NULL)+(24-tm.tm_hour)*3600+(60-tm.tm_min)*60+60-tm.tm_sec;
	
	EventDB *event_db;
	char *filename = CALENDAR_FILE ();	
	if (doshow_appointments==TRUE) {
		event_db = event_db_new (filename);
		if (! event_db)
		{
			g_critical ("Failed to open event database.");
			exit (1);
		}
	}	

	if (doshow_birthdays==TRUE) contacts_db_open(FALSE);
	prepare_birthdays();

	gboolean titletoshow=show_birthdays(vbox,start,"Today");
	count+=add_events(vbox,event_db,start, stop, "Today", titletoshow,0);

	titletoshow=show_birthdays(vbox,stop,"Tomorrow");
	count=add_events(vbox,event_db,stop,stop+3600*24,"Tomorrow",titletoshow, count);
		
	//g_warning("Tomorrow %i",count);
	
	while ((count<doshow_countitems)&&(stop<start+3600*24*5)) {
		char buf2[11];
		//g_warning("Tomorrow %i",count);
		struct tm tm;
		stop=stop+3600*24;
		memset (&tm, 0, sizeof (tm));
		memset (&buf2, 0, sizeof (buf2));
		tm=*localtime(&stop);
	
		strftime (buf2, sizeof(buf2), "%A", &tm);
		
		titletoshow=show_birthdays(vbox,stop,buf2);
		count=add_events(vbox,event_db,stop,stop+3600*24,buf2,titletoshow, count);
	}

	if (doshow_birthdays==TRUE) contacts_db_close ();
	printTime("Events finished");
	
	return count;
	
}

void loadPrefs(){
	gchar *content = NULL;
	gsize length;
	GError *error = NULL;
	
	if (!g_file_get_contents (conffilename, &content, &length, &error)) { // /home/user/apps/
		fprintf (stderr, "ERR: can't read file %s\n", error->message); 
		g_error_free (error);
		return;
	}
        
	if (length>=5) {
		g_warning("loadPrefs");
		g_warning(content+1);
		//GString *save = g_string_new(content);
		//g_warning(save->str[0]);
		if (strncmp(content,"1",1)==0) doshow_birthdays=TRUE; else doshow_birthdays=FALSE;
		if (strncmp(content+1,"1",1)==0) doshow_appointments=TRUE; else doshow_appointments=FALSE;
		if (strncmp(content+2,"1",1)==0) doshow_todos=TRUE; else doshow_todos=FALSE;
		if (strncmp(content+3,"1",1)==0) doshow_buttons=TRUE; else doshow_buttons=FALSE;
		
		if (strncmp(content+4,"0",1)==0) doshow_countitems=2;
		if (strncmp(content+4,"1",1)==0) doshow_countitems=4;
		if (strncmp(content+4,"2",1)==0) doshow_countitems=6;
		if (strncmp(content+4,"3",1)==0) doshow_countitems=8;
		if (strncmp(content+4,"4",1)==0) doshow_countitems=10;
		if (strncmp(content+4,"5",1)==0) doshow_countitems=12;
		
	}
	
	g_free(content);
	
}

void show_all() {
	last_gui_update=time(NULL);
	loadPrefs();
	
	GtkWidget *vbox_todo;
	GtkWidget *vbox_events;
	
	
	if (mainvbox) gtk_widget_destroy(mainvbox);

	mainvbox = gtk_vbox_new (FALSE, 0);
	gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW(scrolled_window), mainvbox );
    
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
	
	if (doshow_buttons==TRUE) {
		printTime("preButtons");
	
		GtkWidget *hbox_buttons;
		hbox_buttons = gtk_hbox_new (FALSE, 1);
		gtk_box_pack_start(GTK_BOX(mainvbox),hbox_buttons,TRUE,TRUE,0);

		button = gtk_button_new_with_label("calen.");  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( calendar_gpestart ), NULL );

		button = gtk_button_new_with_label("todo");  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( todo_gpestart ), NULL );

		button = gtk_button_new_with_label("cont.");  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( contacts_gpestart ), NULL );
	
		/*button = gtk_button_new_with_label("settings");  
		gtk_box_pack_start(GTK_BOX(hbox_buttons),button,TRUE,TRUE,0);
		gtk_signal_connect_object( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( on_menuitem_settings ), NULL );*/
		printTime("postButtons");
	
	}

	gtk_widget_show_all(GTK_WIDGET(mainvbox));
}

void save_prefs() {
	GString *save = g_string_new("");
	if (doshow_birthdays==TRUE) g_string_append(save,"1"); else g_string_append(save,"0");
	if (doshow_appointments==TRUE) g_string_append(save,"1"); else g_string_append(save,"0");
	if (doshow_todos==TRUE) g_string_append(save,"1"); else g_string_append(save,"0");
	if (doshow_buttons==TRUE) g_string_append(save,"1"); else g_string_append(save,"0");
	
	if (doshow_countitems==2) g_string_append(save,"0");
	if (doshow_countitems==4) g_string_append(save,"1");
	if (doshow_countitems==6) g_string_append(save,"2");
	if (doshow_countitems==8) g_string_append(save,"3");
	if (doshow_countitems==10) g_string_append(save,"4");
	if (doshow_countitems==12) g_string_append(save,"5");
	g_string_append(save,"000");
	g_warning(save->str);
	
	if (g_file_set_contents (conffilename,save->str,8,NULL)==TRUE) g_warning("saved");
	
	g_string_free(save,TRUE);
}

static gint options_clicked( GtkWidget      *button, 
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

	save_prefs();
	
	g_string_free(label,TRUE);
	show_all();
}

static options_showcountchanged(GtkComboBox *widget,gpointer user_data) {
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
   
	dialog = gtk_dialog_new_with_buttons ("GPE Summary Options",
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
	
	button = gtk_check_button_new_with_label("Show birthdays (long loading)");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "birthdays");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_birthdays);
	gtk_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label("Show appointments");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "appointments");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_appointments);
	gtk_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label("Show todos");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "todos");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_todos);
	gtk_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_check_button_new_with_label("Show start buttons");
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_widget_set_name(button, "buttons");	
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),doshow_buttons);
	gtk_signal_connect( GTK_OBJECT( button ), "clicked",   GTK_SIGNAL_FUNC( options_clicked ), NULL );
	
	button = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~2 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~4 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~6 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~8 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~10 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~12 appointments/todos");
	gtk_combo_box_append_text(GTK_COMBO_BOX(button),"Show ~14 appointments/todos");
	gtk_combo_box_set_active(GTK_COMBO_BOX(button),round((doshow_countitems/2)-1));
	gtk_widget_set_name(button, "buttons");	
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),button);
	gtk_signal_connect( GTK_OBJECT( button ), "changed",   GTK_SIGNAL_FUNC( options_showcountchanged ), NULL );
	
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
}

int main (int argc, char *argv[])
{
    HildonProgram *program;

    osso = osso_initialize ("gpesummary", "0.4", FALSE, NULL);
    if (! osso)
	    return -1;


    gtk_init(&argc, &argv);

    program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("GPE Summary");

    window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program, window);
    
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_container_add( GTK_CONTAINER( window ), scrolled_window );	

    show_all();
    
    gtk_widget_show_all(GTK_WIDGET(window));

    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(GTK_WIDGET(window), "focus-in-event", G_CALLBACK(focus_in), NULL);

    save_prefs();

    gtk_main();


    return 0;
}





//Home Applet stuff
void *
		hildon_home_applet_lib_initialize (void *state_data,
		int *state_size,
		GtkWidget **widget)
{

	osso = osso_initialize ("gpesummary", "0.4", FALSE, NULL);
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
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (mainwidget), alignment);
	gtk_container_add( GTK_CONTAINER( alignment ), scrolled_window );	

	show_all();

	//button = gtk_button_new_with_label("todo");  
	//gtk_box_pack_start(GTK_BOX(mainwidget),button,TRUE,TRUE,0);
    

	gtk_widget_show_all(GTK_WIDGET(mainwidget));
	
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
	show_all();
	//g_warning("gui updated");
	return;
}

static gint prefs_ok( GtkWidget      *button, 
			  GdkEventButton *event,
			  gpointer user_data ) {
				  
	gtk_widget_hide(prefsvbox);
	if (prefsvbox) gtk_widget_destroy(prefsvbox);
	
}

GtkWidget *
		hildon_home_applet_lib_settings (void *data, GtkWindow *parent)
{

	settingswindow = parent;
    
	GtkWidget *settings;

	settings = gtk_menu_item_new_with_label
			(("GPE Summary"));
	g_signal_connect (settings, "activate",
			  G_CALLBACK (on_menuitem_settings), NULL);

	return settings;

}
