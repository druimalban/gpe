#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#define _XOPEN_SOURCE /* Pour GlibC2 */
#include <time.h>
#include "applets.h"
#include "timeanddate.h"
#include "suid.h"

static struct 
{
  GtkWidget *applet;
  GtkWidget *datel;
  GtkWidget *cal;
  GtkWidget *timel;
  GtkWidget *hbox;
  GtkWidget *h;
  GtkWidget *m;
  GtkWidget *s;
  GtkWidget *internetserver;
  GtkWidget *internet;
}self;

GtkAttachOptions table_attach_left_col_x;
GtkAttachOptions table_attach_left_col_y;
GtkAttachOptions table_attach_right_col_x;
GtkAttachOptions table_attach_right_col_y;
GtkJustification table_justify_left_col;
GtkJustification table_justify_right_col;
guint border_width;
guint col_spacing;
guint row_spacing;
guint widget_padding_x;
guint widget_padding_y;
guint widget_padding_y_even;
guint widget_padding_y_odd;

void
InitSpacingsTime () {
  /* 
   * GTK_EXPAND  the widget should expand to take up any extra space
                 in its container that has been allocated.
   * GTK_SHRINK  the widget should shrink as and when possible.
   * GTK_FILL    the widget should fill the space allocated to it.
   */
  
  /*
   * GTK_SHRINK to make it as small as possible, but use GTK_FILL to
   * let it fill on the left side (so that the right alignment
   * works:
   */ 
  table_attach_left_col_x = GTK_FILL; 
  table_attach_left_col_y = 0;
  table_attach_right_col_x = GTK_EXPAND | GTK_FILL;
  //table_attach_right_col_x = GTK_SHRINK | GTK_EXPAND | GTK_FILL;
  table_attach_right_col_y = GTK_FILL;
  
  /*
   * GTK_JUSTIFY_LEFT
   * GTK_JUSTIFY_RIGHT
   * GTK_JUSTIFY_CENTER (the default)
   * GTK_JUSTIFY_FILL
   */
  table_justify_left_col = GTK_JUSTIFY_LEFT;
  table_justify_right_col = GTK_JUSTIFY_RIGHT;

  border_width = 6;
  col_spacing = 6;
  row_spacing = 6;
  widget_padding_x = 0; /* add space with col_spacing */
  widget_padding_y = 0; /* add space with row_spacing */
  widget_padding_y_even = 6; /* padding in y direction for widgets in an even row */
  widget_padding_y_odd  = 6; /* padding in y direction for widgets in an odd row  */
}

void GetInternetTime()
{
  fprintf(suidout,"NTPD\n%s\n",gtk_entry_get_text(GTK_ENTRY(self.internetserver)));
  fflush(suidout);
  printf("\n");
  sleep(1);
  Time_Restore();
}

GtkWidget *Time_Build_Objects()
{  
  GtkObject *adj;
  // get the time and the date.
  time_t t = time(NULL);
  struct tm *tsptr = localtime(&t);
  struct tm ts = *tsptr;     /* gtk_cal seems to modify the ptr
				returned by localtime, so we duplicate it.. */

  ts.tm_year+=1900;

  if(ts.tm_year <2002)
    ts.tm_year=2002;

  InitSpacingsTime ();

  self.applet = gtk_vbox_new(FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (self.applet), border_width);

  self.datel = gtk_label_new("Date:");
  gtk_box_pack_start(GTK_BOX(self.applet),self.datel, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (self.datel), 0, 0.5);

  self.cal = gtk_calendar_new();
    gtk_calendar_select_month(GTK_CALENDAR(self.cal),ts.tm_mon,ts.tm_year);
  gtk_calendar_select_day(GTK_CALENDAR(self.cal),ts.tm_mday);

  gtk_box_pack_start(GTK_BOX(self.applet),self.cal, FALSE, FALSE, 0);

  self.timel = gtk_label_new("Time:");
  gtk_box_pack_start(GTK_BOX(self.applet),self.timel, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (self.timel), 0, 0.5);

  self.hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(self.applet),self.hbox, FALSE, FALSE, 0);

  adj = gtk_adjustment_new(ts.tm_hour,0,24,1,6,6);
  self.h = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.h);

  adj = gtk_adjustment_new(ts.tm_min,0,60,1,15,15);
  self.m = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.m);

  adj = gtk_adjustment_new(ts.tm_sec,0,60,1,15,15);
  self.s = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.s);

  self.internetserver = gtk_entry_new_with_max_length(30);
  gtk_entry_set_text(GTK_ENTRY(self.internetserver),"time.apple.com");

  gtk_box_pack_start(GTK_BOX(self.applet),self.internetserver, FALSE, FALSE, 0);

  self.internet = gtk_button_new_with_label("Get time from network");
  gtk_signal_connect (GTK_OBJECT(self.internet), "clicked",
		      (GtkSignalFunc) GetInternetTime, NULL);

  gtk_box_pack_start(GTK_BOX(self.applet),self.internet, FALSE, FALSE, 0);


  gtk_widget_show_all(self.applet);
  return self.applet;

}
void Time_Free_Objects()
{
}

void Time_Save()
{
  guint year,month,day, h, m,s;
  struct tm tm;
  time_t t;
  gtk_calendar_get_date(GTK_CALENDAR(self.cal),&year,&month,&day);
  h=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.h));
  m=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.m));
  s=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(self.s));
  tm.tm_mday=day;
  tm.tm_mon=month;
  tm.tm_year=year-1900;
  tm.tm_hour=h;
  tm.tm_min=m;
  tm.tm_sec=s;
  tm.tm_isdst = 0;
  t=mktime(&tm);
  fprintf(suidout,"STIM %ld",t);
  fflush(suidout);
  //  stime(&t);
}
void Time_Restore()
{
  time_t t = time(NULL);
  struct tm *tsptr = localtime(&t);
  struct tm ts = *tsptr;     /* gtk_cal seems to modify the ptr
				returned by localtime, so we duplicate it.. */
  gtk_calendar_select_month(GTK_CALENDAR(self.cal),ts.tm_mon,ts.tm_year+1900);
  gtk_calendar_select_day(GTK_CALENDAR(self.cal),ts.tm_mday);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.h),ts.tm_hour);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.m),ts.tm_min);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(self.s),ts.tm_sec);

}
