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
  GtkWidget *cal;
  GtkWidget *hbox;
  GtkWidget *h;
  GtkWidget *m;
  GtkWidget *s;
  GtkWidget *internetserver;
  GtkWidget *internet;
}self;

guint gpe_catspacing = 18/2;
guint gpe_border = 12/2;
guint gpe_boxspacing = 6/2;
gchar *gpe_catindent = "  "; /* Gnome 2 uses four spaces */

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

  if(ts.tm_year < 2002)
    ts.tm_year=2002;

  self.categories = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_container_set_border_width (GTK_CONTAINER (self.categories), gpe_border);

  /* -------------------------------------------------------------------------- */
  self.catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox1, TRUE, TRUE, 0);

  self.catlabel1 = gtk_label_new (_("Date")); /* FIXME: GTK2: make this bold */
  gtk_box_pack_start (GTK_BOX (self.catvbox1), self.catlabel1, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (self.catlabel1), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (self.catlabel1), 0, 0.5);

  self.catconthbox1 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (self.catvbox1), self.catconthbox1, TRUE, TRUE, 0);

  self.catindentlabel1 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (self.catconthbox1), self.catindentlabel1, FALSE, FALSE, 0);

  self.controlvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.catconthbox1), self.controlvbox1, TRUE, TRUE, 0);
  
  self.cal = gtk_calendar_new ();
  gtk_calendar_select_month (GTK_CALENDAR (self.cal), ts.tm_mon, ts.tm_year);
  gtk_calendar_select_day (GTK_CALENDAR (self.cal), ts.tm_mday);
  
  gtk_box_pack_start (GTK_BOX (self.controlvbox1), self.cal, FALSE, FALSE, 0);

  /* -------------------------------------------------------------------------- */
  self.catvbox2 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox2, TRUE, TRUE, 0);

  self.catlabel2 = gtk_label_new (_("Time")); /* FIXME: GTK2: make this bold */
  gtk_box_pack_start (GTK_BOX (self.catvbox2), self.catlabel2, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (self.catlabel2), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (self.catlabel2), 0, 0.5);

  self.catconthbox2 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (self.catvbox2), self.catconthbox2, TRUE, TRUE, 0);

  self.catindentlabel2 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (self.catconthbox2), self.catindentlabel2, FALSE, FALSE, 0);

  self.controlvbox2 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.catconthbox2), self.controlvbox2, TRUE, TRUE, 0);
  
  self.hbox = gtk_hbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.controlvbox2), self.hbox, FALSE, FALSE, 0);

  adj = gtk_adjustment_new(ts.tm_hour,0,24,1,6,6);
  self.h = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.h);

  adj = gtk_adjustment_new(ts.tm_min,0,60,1,15,15);
  self.m = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.m);

  adj = gtk_adjustment_new(ts.tm_sec,0,60,1,15,15);
  self.s = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
  gtk_container_add(GTK_CONTAINER(self.hbox),self.s);

  /* -------------------------------------------------------------------------- */
  self.catvbox3 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox3, TRUE, TRUE, 0);

  self.catlabel3 = gtk_label_new (_("Network")); /* FIXME: GTK2: make this bold */
  gtk_box_pack_start (GTK_BOX (self.catvbox3), self.catlabel3, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (self.catlabel3), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (self.catlabel3), 0, 0.5);

  self.catconthbox3 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (self.catvbox3), self.catconthbox3, TRUE, TRUE, 0);

  self.catindentlabel3 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (self.catconthbox3), self.catindentlabel3, FALSE, FALSE, 0);

  self.controlvbox3 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.catconthbox3), self.controlvbox3, TRUE, TRUE, 0);
  
  self.internetserver = gtk_entry_new_with_max_length(30);
  gtk_entry_set_text(GTK_ENTRY(self.internetserver),"time.apple.com");

  gtk_box_pack_start(GTK_BOX(self.controlvbox3),self.internetserver, FALSE, FALSE, 0);

  self.internet = gtk_button_new_with_label("Get time from network");
  gtk_signal_connect (GTK_OBJECT(self.internet), "clicked",
		      (GtkSignalFunc) GetInternetTime, NULL);

  gtk_box_pack_start(GTK_BOX(self.controlvbox3),self.internet, FALSE, FALSE, 0);

  gtk_widget_show_all(self.categories);
  return self.categories;
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
