/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003 - 2006  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Time and date settings module.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <libintl.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "applets.h"
#include "timeanddate.h"
#include "suid.h"
#include "misc.h"
#include "timezones.h"

#include <gpe/spacing.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/gpetimesel.h>
#include <gpe/infoprint.h>


/* --- local types and constants --- */

gchar *Ntpservers[6]=  {
	"pool.ntp.org", 
	"time.handhelds.org",
	"time.apple.com",
	"ptbtime1.ptb.de",
	"ntp2c.mcc.ac.uk",
	"ntppub.tamu.edu"
};

enum {
	COL_VIEW_ID,
	COL_VIEW_NAME,	
	N_VIEW_COLUMNS
};

#define SCREENSAVER_RESET_CMD "xset s reset"
#define TZ_MAXINDEX 10
  
/* --- module global variables --- */
  
static struct 
{
	GtkWidget *categories;
	GtkWidget *catvbox1;
	GtkWidget *catlabel1;
	GtkWidget *catconthbox1;
	GtkWidget *catindentlabel1;
	GtkWidget *controlvbox1;
	GtkWidget *catvbox2;
	GtkWidget *catlabel2;
	GtkWidget *catconthbox2;
	GtkWidget *catindentlabel2;
	GtkWidget *controlvbox2;
	GtkWidget *catvbox3;
	GtkWidget *catlabel3;
	GtkWidget *catconthbox3;
	GtkWidget *catindentlabel3;
	GtkWidget *controlvbox3;
	GtkWidget *catvbox4;
	GtkWidget *catlabel4;
	GtkWidget *catconthbox4;
	GtkWidget *catindentlabel4;
	GtkWidget *controlvbox4;
	GtkWidget *cal;
	GtkWidget *hbox;
	GtkWidget *tsel;
	GtkWidget *ntpserver;
	GtkWidget *internet;
	GtkWidget *timezoneArea;
	GtkWidget *timezone;
	GtkWidget *usedst;
	GtkWidget *dsth;
	GtkWidget *dstm;
	GtkWidget *defaultdst;
	GtkWidget *dstlh;
	GtkWidget *dstlm;
	GtkWidget *offsetlabel;
} self;

static int trc = 0;  // countdown for waiting for time change
static int tid = 0;  // timeout id
static int isdst;    // is a dst active?
static int need_warning = FALSE;
static int nonroot_mode = FALSE;
static gchar ***timezoneAreaArray = NULL;
static gint timezoneAreaArray_len = 0;

/* --- local intelligence --- */

static void
read_installed_timezones (void)
{
	DIR *dir;
	gint idx = 0;
	
	while (timezoneNameArray [idx])
	{
		gchar *zonedir = g_strdup_printf ("/usr/share/zoneinfo/%s", 
		                                  timezoneNameArray [idx]);
		gchar **znamearray = NULL;
		gint len = 0;
		
		dir = opendir (zonedir);
		if (dir)
		{
			struct dirent *entry;
      		while ((entry = readdir (dir)))
			{
				if (entry->d_name[0] == '.') 
					continue;
			  
				len++;
				znamearray = g_realloc (znamearray, (len + 1) * sizeof (gchar *));
				znamearray[len - 1] = g_strdup (entry->d_name);
				znamearray[len] = NULL;
			}
			closedir (dir);
		}
		
		timezoneAreaArray_len++;
		timezoneAreaArray = g_realloc (timezoneAreaArray,  (timezoneAreaArray_len + 1) * sizeof (gchar **));
		timezoneAreaArray[timezoneAreaArray_len - 1] = znamearray;
		timezoneAreaArray[timezoneAreaArray_len] = NULL;
			
		g_free (zonedir);
		idx++;
	}
}

/* find current timezone definition */

gchar* 
get_current_tz(void)
{
	gchar *pos;
	gchar *result;	
	gchar *cont = NULL;
	GError *err = NULL;

	if (g_file_get_contents ("/etc/timezone", &cont, NULL, &err))
	{
		pos = strstr (cont, "\n");
		if (pos)
			pos[0] = 0;
		result = g_strdup (cont);
		g_free (cont);
	}
	else
	{
		g_printerr ("Could not read timezone file (%s), using default.\n", err->message);
		g_error_free (err);
		result = g_strdup ("Europe/Berlin");
	}
	return result;  
}

/* This function sets the system timezone */

void
set_timezone (gchar *zone)
{
	FILE *ftimezone;
	
	ftimezone = fopen ("/etc/timezone", "w");
	if (ftimezone)
	{
		fprintf (ftimezone, "%s\n", zone);
		fchmod(fileno(ftimezone), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		fclose (ftimezone);
	}
	else
		g_printerr("Error: Could not write to /etc/timezone\n");
}

gboolean 
refresh_time(void)
{
	static char str[256];
	struct pollfd pfd[1];
	gboolean ret = FALSE;
	Display *dpy = GDK_DISPLAY();
		
	Time_Restore();
	memset(str, 0, 256);
	
	pfd[0].fd = suidinfd;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll(pfd, 1, 0))
	{
		if (fgets (str, 255, suidin))
		{
			if (strstr(str, "<success>"))
				gpe_popup_infoprint (dpy, 
			                         _("Time adjusted from network."));
			else
				gpe_error_box(_("Adjusting time from network failed."));

		}
		ret = TRUE;
	}

	trc--;
	if (!trc) 
		ret = TRUE;
	if (ret)
	{
		gtk_widget_set_sensitive(self.internet, TRUE);
		return FALSE;
	}
	system (SCREENSAVER_RESET_CMD);
	return (TRUE);
}

void 
GetInternetTime(void)
{
	gchar	*srcSelected;
	
	gtk_widget_set_sensitive(self.internet,FALSE);
	srcSelected	= gtk_combo_box_get_active_text(GTK_COMBO_BOX(self.ntpserver));
	suid_exec("NTPD", srcSelected);
	trc = 60;
	tid = g_timeout_add(500, (GSourceFunc)refresh_time, NULL);
}

static void
do_tabchange (GtkNotebook * notebook,
	      GtkNotebookPage * page, guint page_num, gpointer user_data)
{
	if (page_num == 1)
		need_warning = TRUE;
}

gchar * 
getSelectedTimezoneText (void)
{
	gint		curIndex;
	gint		curAreaIndex;
	gchar		*retVal = NULL;
	
	curAreaIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezoneArea));
	curIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezone));
	if ((curAreaIndex > -1) && (curIndex > -1))
	{
		retVal = g_strconcat (timezoneNameArray[curAreaIndex], "/",
		                          timezoneAreaArray[curAreaIndex][curIndex], NULL);
	}
	return retVal;
}

void 
timezonearea_combo_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkListStore	*timezoneModel;
	GtkTreeIter	timezoneIter;
	gint		curIndex;
	gchar		**tzPointer;
	guint		idx;

	curIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezoneArea));
	if (curIndex > -1)
	{
		timezoneModel		= (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(self.timezone));
		/* clear previous timezone texts */
		gtk_list_store_clear(timezoneModel);
		
		/* add new timezone texts */
		idx			= 0;
		tzPointer	= timezoneAreaArray[curIndex];		
		while(*tzPointer != NULL) {
       			gtk_list_store_append(timezoneModel, &timezoneIter);
				gtk_list_store_set(timezoneModel, &timezoneIter,
					COL_VIEW_ID, idx,
					COL_VIEW_NAME, *tzPointer,
					-1);
			tzPointer++;
			idx++;
		}
		/* select first from the list by default */
		gtk_combo_box_set_active(GTK_COMBO_BOX (self.timezone), 0);
	}	
}

/*
* Method for finding timezone area index by using timezone name as a key.
* Method could be optimized by only searching from "Other" area and
* if not found from there then comparing the timezone area names with the start part of the
* timezone area name. But this method is not done very often and this way it was easier to implement
* for me so...
*
* If area is not found -1 is returned.
*/
gint getTimezoneAreaIndexByTimezoneName(gchar *tzNameParam)
{
	gint		retVal = -1;
	gchar		**tzAreasPointer;
	guint		idx = 0;
	gboolean	curFound = FALSE;
	gchar		*searchname;

	searchname = strstr (tzNameParam, "/");
    
    if (!searchname)
 		searchname = tzNameParam;
    else
        searchname++;

	while (timezoneNameArray[idx] != NULL) 
	{
		tzAreasPointer	= timezoneAreaArray[idx];
    	while (tzAreasPointer && (*tzAreasPointer != NULL)) 
		{
   			if (strstr(*tzAreasPointer, searchname)) 
			{
				retVal = idx;
   				curFound = TRUE;
   				break;
   			}
			tzAreasPointer++;
		}
		if (curFound == TRUE)
		{
			break;
		}
		idx++;
	}
	return retVal;
}

/**
* If timezone is not found from the timezone area, -1 is returned.
*/
gint 
getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(guint tzAreaIndexParam, gchar *tzNameParam)
{
	gint	retVal = -1;
	gchar	**tzAreasPointer;
	guint	idx = 0;
	gchar	*searchname;

	searchname = strstr (tzNameParam, "/") + 1;
	
	if ((gint)searchname == 1)
		searchname = tzNameParam;
	
	if (tzAreaIndexParam >= 0)
	{
    		tzAreasPointer	= timezoneAreaArray[tzAreaIndexParam];
    		while (tzAreasPointer && (*tzAreasPointer != NULL))
			{
    			if (strstr(*tzAreasPointer, searchname)) 
				{
    				retVal	= idx;
    				break;
    			}
				tzAreasPointer++;
				idx++;
			}
	}
	return retVal;
}

/* --- gpe-conf interface --- */

GtkWidget *
Time_Build_Objects(gboolean nonroot)
{  
	guint idx;

	GtkWidget *table;
	GtkTooltips *tooltips;
	GtkWidget *notebook, *label;
	time_t t;
	struct tm *tsptr;
	struct tm ts;     /* gtk_cal seems to modify the ptr
				returned by localtime, so we duplicate it.. */

	guint gpe_border     = gpe_get_border ();
	guint gpe_boxspacing = gpe_get_boxspacing ();
  
	gchar *fstr = NULL;
  
	guint		selTimezoneAreaIndx;
	guint		selTimezoneIndx;
	gchar 		**timezoneAreasPointer;	
	GtkListStore 	*timezoneAreaModel;
	GtkTreeIter  	timezoneAreaIter;
	GtkCellRenderer *timezoneAreaRend;
	
	GtkListStore 	*timezoneModel;
	GtkTreeIter  	timezoneIter;
	GtkCellRenderer *timezoneRend;	
  
	nonroot_mode = nonroot;
  
	// get the time and the date.
	time(&t);
	tsptr = localtime(&t);
	ts = *tsptr;
	ts.tm_year+=1900;

	if(ts.tm_year < 2006)
		ts.tm_year=2006;
	isdst = ts.tm_isdst;

	read_installed_timezones();
	
	tooltips = gtk_tooltips_new ();
  
	notebook = gtk_notebook_new();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_border);

	if (!nonroot_mode)
	{
		label = gtk_label_new(_("Time & Date"));
		self.categories = table = gtk_table_new (6, 3, FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
		gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
		gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
		gtk_widget_set_size_request(notebook, 140, -1);
	  
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self.categories, label);
	  
		g_object_set_data(G_OBJECT(notebook), "tooltips", tooltips);
	  	
		self.catlabel1 = gtk_label_new (NULL); 
		fstr = g_strdup_printf("%s %s %s","<b>",_("Date"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel1),fstr); 
		g_free(fstr);
		gtk_table_attach (GTK_TABLE (table), self.catlabel1, 
		                  0, 3,	0, 1, GTK_FILL,	GTK_FILL, 0, 0);
	
		gtk_misc_set_alignment (GTK_MISC (self.catlabel1), 0.0, 0.9);
	  
		self.cal = gtk_date_combo_new ();
		gtk_entry_set_activates_default(GTK_ENTRY(GTK_DATE_COMBO(self.cal)->entry), TRUE);
		gtk_calendar_select_month (GTK_CALENDAR (GTK_DATE_COMBO(self.cal)->cal), ts.tm_mon, ts.tm_year);
		gtk_calendar_select_day (GTK_CALENDAR (GTK_DATE_COMBO(self.cal)->cal), ts.tm_mday);
		gtk_table_attach (GTK_TABLE (table), self.cal,
		                  0, 3, 1, 2, GTK_FILL, 0, 3, 0);
	  
		gtk_tooltips_set_tip (tooltips, self.cal, 
		                      _("Enter current date here or use button to select."), NULL);
	
		self.catlabel2 = gtk_label_new (NULL); 
		fstr = g_strdup_printf("%s %s %s","<b>",_("Time"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel2),fstr); 
		g_free(fstr);
		gtk_table_attach (GTK_TABLE (table), self.catlabel2, 0, 3, 2, 3,
		                  GTK_FILL, GTK_FILL, 0, 0);
		gtk_misc_set_alignment (GTK_MISC (self.catlabel2), 0.0, 0.9);
	
		self.tsel = gpe_time_sel_new();
		gpe_time_sel_set_time(GPE_TIME_SEL(self.tsel),(guint)ts.tm_hour, (guint)ts.tm_min);
		gtk_table_attach (GTK_TABLE (table), self.tsel, 
		                  0, 3,	3, 4, GTK_FILL,	0, 3, 0);
	
		self.catlabel3 = gtk_label_new (NULL);
		fstr = g_strdup_printf("%s %s %s","<b>",_("Network"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel3),fstr); 
		g_free(fstr);
	  
		gtk_table_attach (GTK_TABLE (table), self.catlabel3, 
		                  0, 3, 4, 5, GTK_FILL,	GTK_FILL, 0, 0);
		gtk_misc_set_alignment (GTK_MISC (self.catlabel3), 0.0, 0.9);
	
	
		self.ntpserver = gtk_combo_box_new_text ();
		for (idx=0; idx<5; idx++) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (self.ntpserver), Ntpservers[idx]);
		}
		gtk_combo_box_set_active (GTK_COMBO_BOX (self.ntpserver), 0);
		
		gtk_table_attach (GTK_TABLE (table), self.ntpserver, 
		                  0, 3,	5, 6, GTK_FILL, GTK_FILL, 3, 0);
 
		gtk_tooltips_set_tip (tooltips, self.ntpserver, 
		                      _("Select the timeserver to use to set the clock."), NULL);
	
		self.internet = gtk_button_new_with_label(_("Get time from network"));
		g_signal_connect (G_OBJECT(self.internet), "clicked",
				  G_CALLBACK(GetInternetTime), NULL);
		gtk_widget_set_sensitive(self.internet,is_network_up());
	  
		gtk_table_attach (GTK_TABLE (table), self.internet, 
		                  0, 3, 6, 7, GTK_FILL | GTK_EXPAND, GTK_FILL, gpe_border, gpe_border);
		gtk_tooltips_set_tip (tooltips, self.internet,
		                      _("If connected to the Internet, you may press this button to set the time on this device using the timeserver above."), NULL);
	} // root_mode

	label = gtk_label_new(_("Timezone"));
	self.catvbox4 = table = gtk_table_new (8, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
  
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), self.catvbox4, label);
	
	// create TimeZone label
	self.catlabel4 = gtk_label_new (NULL);
	fstr = g_strdup_printf("<b>%s</b>", _("Timezone"));
	gtk_label_set_markup (GTK_LABEL(self.catlabel4), fstr); 
	g_free(fstr);  
	gtk_misc_set_alignment (GTK_MISC (self.catlabel4), 0.0, 0.9);
	gtk_table_attach (GTK_TABLE (table), self.catlabel4, 
	                  0, 4, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
			
	// timezone area combobox
	fstr = get_current_tz();
    
	selTimezoneAreaIndx	= getTimezoneAreaIndexByTimezoneName(fstr);
	if (selTimezoneAreaIndx == -1)
	{
		g_free(fstr);
		fstr = g_strdup("Europe/Berlin");
		selTimezoneAreaIndx	= getTimezoneAreaIndexByTimezoneName(fstr);
	}
	if (selTimezoneAreaIndx == -1)
	{
		selTimezoneAreaIndx	= 0;
		selTimezoneIndx		= 0;
	}
	else
	{
		selTimezoneIndx	= getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(selTimezoneAreaIndx, fstr);
	}
	g_free(fstr);
	
	timezoneAreaModel	= gtk_list_store_new(N_VIEW_COLUMNS,
						G_TYPE_INT,
						G_TYPE_STRING);
	idx	= 0;
	while (timezoneNameArray[idx] != NULL) 
	{
    	gtk_list_store_append(timezoneAreaModel, &timezoneAreaIter);
	   	gtk_list_store_set(timezoneAreaModel, &timezoneAreaIter,
               			COL_VIEW_ID, idx,
                       	COL_VIEW_NAME, timezoneNameArray[idx], -1);
		idx++;
	}
	label = gtk_label_new(_("Area"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,	0, 1, 1, 2,	GTK_FILL | GTK_EXPAND, 
	                  GTK_FILL, 0, 0);
	
	self.timezoneArea	= gtk_combo_box_new_with_model(GTK_TREE_MODEL(timezoneAreaModel));
	timezoneAreaRend	= gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self.timezoneArea), 
				timezoneAreaRend, 
				TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self.timezoneArea), 
				timezoneAreaRend,
               			"text", COL_VIEW_NAME,
               			NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX (self.timezoneArea), selTimezoneAreaIndx);
	gtk_tooltips_set_tip(tooltips, self.timezoneArea, _("Select your current timezone area here. The setting applies after the next login."), NULL);
	gtk_table_attach (GTK_TABLE (table), self.timezoneArea, 
	                  1, 2, 1, 2, GTK_FILL | GTK_EXPAND, 0, 0, 0);
	g_signal_connect(G_OBJECT(GTK_COMBO_BOX(self.timezoneArea)), "changed",
	                 G_CALLBACK(timezonearea_combo_changed), NULL);
	
	label = gtk_label_new(_("Zone"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,	0, 1, 2, 3,	GTK_FILL | GTK_EXPAND, 
	                  0, 0, 0);
	// timezone combobox
	timezoneModel	= gtk_list_store_new(N_VIEW_COLUMNS,
	                                     G_TYPE_INT, G_TYPE_STRING);
	idx			= 0;
	timezoneAreasPointer	= timezoneAreaArray[selTimezoneAreaIndx];
    if (timezoneAreasPointer)
        while(timezoneAreasPointer[idx] != NULL) 
    	{
    		gtk_list_store_append(timezoneModel, &timezoneIter);
    		gtk_list_store_set(timezoneModel, &timezoneIter,
    					COL_VIEW_ID, idx,
    					COL_VIEW_NAME, timezoneAreasPointer[idx], -1);
    		idx++;
    	}
	self.timezone	= gtk_combo_box_new_with_model(GTK_TREE_MODEL(timezoneModel));
	timezoneRend	= gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self.timezone),
	                           timezoneRend, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self.timezone),
	                               timezoneRend, "text", COL_VIEW_NAME, NULL);	
	gtk_tooltips_set_tip (tooltips, self.timezone, 
	                      _("Select your current timezone here. The setting applies after the next login."), NULL);
	gtk_table_attach (GTK_TABLE (table), self.timezone,
	                  1, 2, 2, 3, GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_combo_box_set_active(GTK_COMBO_BOX (self.timezone), selTimezoneIndx);
	
	g_signal_connect_after (G_OBJECT (notebook), "switch-page",
				G_CALLBACK (do_tabchange), NULL);
	gtk_widget_show_all(notebook);
  
	return notebook;
}

void 
Time_Free_Objects(void)
{
}

void 
Time_Save(void)
{
	guint year,month,day, h, m,s;
	struct tm tm;
	time_t t;
	gchar *par;
	GtkWidget *dialog;	

  	par	= getSelectedTimezoneText();
	/* set timezone */
	if (par != NULL) {
		suid_exec("STZO", par);
		g_free (par);
  	}
	if (!nonroot_mode)
	{
		year = GTK_DATE_COMBO(self.cal)->year;
		month = GTK_DATE_COMBO(self.cal)->month;
		day = GTK_DATE_COMBO(self.cal)->day;
		gpe_time_sel_get_time(GPE_TIME_SEL(self.tsel),&h,&m);
		s = 0;	 
		memset (&tm, 0, sizeof (struct tm));
		tm.tm_mday=day;
		tm.tm_mon=month;
		tm.tm_year=year-1900;
		tm.tm_hour=h;
		tm.tm_min=m;
		tm.tm_sec=s;
		tm.tm_isdst = -1;
		t = timelocal (&tm);
		par = g_strdup_printf ("%ld", t);
		suid_exec("STIM", par);  
		g_free(par);
		usleep(300000);
		system (SCREENSAVER_RESET_CMD);
	}
	if (need_warning)
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (mainw),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("To make timezone settings take effect, "\
               					"you'll need to log out and log in again."));
						gtk_dialog_run(GTK_DIALOG(dialog));
	}
}

void 
Time_Restore(void)
{
	time_t t;
	struct tm *tsptr;
	struct tm ts;     /* gtk_cal seems to modify the ptr
			     returned by localtime, so we duplicate it.. */
	if (!nonroot_mode)
	{
		time(&t);
		tsptr = localtime(&t);
		ts = *tsptr;
		gtk_calendar_select_month(GTK_CALENDAR(GTK_DATE_COMBO(self.cal)->cal),ts.tm_mon,ts.tm_year+1900);
		gtk_calendar_select_day(GTK_CALENDAR(GTK_DATE_COMBO(self.cal)->cal),ts.tm_mday);
		gpe_time_sel_set_time(GPE_TIME_SEL(self.tsel),(guint)ts.tm_hour, (guint)ts.tm_min);
	}
}
