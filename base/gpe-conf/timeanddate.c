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

void GetInternetTime()
{
  system_printf("ntpdate %s",gtk_entry_get_text(GTK_ENTRY(self.internetserver)));
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

  self.applet = gtk_vbox_new(FALSE,0);
  self.datel = gtk_label_new("date:");
  gtk_box_pack_start(GTK_BOX(self.applet),self.datel,TRUE, TRUE, 0);

  self.cal = gtk_calendar_new();
    gtk_calendar_select_month(GTK_CALENDAR(self.cal),ts.tm_mon,ts.tm_year);
  gtk_calendar_select_day(GTK_CALENDAR(self.cal),ts.tm_mday);


  gtk_box_pack_start(GTK_BOX(self.applet),self.cal,TRUE, TRUE, 0);

  self.timel = gtk_label_new("time:");
  gtk_box_pack_start(GTK_BOX(self.applet),self.timel,TRUE, TRUE, 0);

  self.hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(self.applet),self.hbox,TRUE, TRUE, 0);

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

  gtk_box_pack_start(GTK_BOX(self.applet),self.internetserver,TRUE, TRUE, 0);

  self.internet = gtk_button_new_with_label("Get from network");
  gtk_signal_connect (GTK_OBJECT(self.internet), "clicked",
		      (GtkSignalFunc) GetInternetTime, NULL);

  gtk_box_pack_start(GTK_BOX(self.applet),self.internet,TRUE, TRUE, 0);


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
  tm.tm_mon=month-1;
  tm.tm_year=year-1900;
  tm.tm_hour=h;
  tm.tm_min=m;
  tm.tm_sec=s;
  tm.tm_isdst = 0;
  t=mktime(&tm);
  stime(&t);
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
