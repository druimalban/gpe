#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* For GlibC2 */
#endif
#include <time.h>
#include "applets.h"
#include "timeanddate.h"
#include "suid.h"

#include <gpe/spacing.h>
#include <gpe/errorbox.h>

gchar *Ntpservers[6]=
  {
    "time.handhelds.org",
    "time.apple.com",
    "time.esec.com.au",
    "ptbtime1.ptb.de",
    "ntp2c.mcc.ac.uk",
    "ntppub.tamu.edu"
  };

gchar *timezones[3]=
  {
    "UTC",
    "MET",
    "GMT"
  };

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
  GtkWidget *h;
  GtkWidget *m;
  GtkWidget *s;
  GtkWidget *ntpserver;
  GtkWidget *internet;
  GtkWidget *timezone;
} self;

void GetInternetTime()
{
  suid_exec("NTPD",gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (self.ntpserver)->entry)));
  sleep(1);
  Time_Restore();
}

GtkWidget *Time_Build_Objects()
{  
  guint idx;
  GList *ntpsrv = NULL;
  GList *tzones = NULL;
 
  GtkObject *adj;
  // get the time and the date.
  time_t t = time(NULL);
  struct tm *tsptr = localtime(&t);
  struct tm ts = *tsptr;     /* gtk_cal seems to modify the ptr
				returned by localtime, so we duplicate it.. */

  guint gpe_catspacing = gpe_get_catspacing ();
  gchar *gpe_catindent = gpe_get_catindent ();
  guint gpe_border     = gpe_get_border ();
  guint gpe_boxspacing = gpe_get_boxspacing ();
  
  gchar *fstr = NULL;
  guint mark = 0;
  
  ts.tm_year+=1900;

  if(ts.tm_year < 2002)
    ts.tm_year=2002;


  self.categories = gtk_vbox_new (FALSE, gpe_catspacing);
  gtk_container_set_border_width (GTK_CONTAINER (self.categories), gpe_border);

  /* -------------------------------------------------------------------------- */
  self.catvbox1 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox1, TRUE, TRUE, 0);

  self.catlabel1 = gtk_label_new (NULL); 
  fstr = g_strdup_printf("%s %s %s","<b>",_("Date"),"</b>");
  gtk_label_set_markup (GTK_LABEL(self.catlabel1),fstr); 
  g_free(fstr);
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

  self.catlabel2 = gtk_label_new (NULL); 
  fstr = g_strdup_printf("%s %s %s","<b>",_("Time"),"</b>");
  gtk_label_set_markup (GTK_LABEL(self.catlabel2),fstr); 
  g_free(fstr);
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
  self.catvbox4 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox4, TRUE, TRUE, 0);

  self.catlabel4 = gtk_label_new (NULL);
  fstr = g_strdup_printf("%s %s %s","<b>",_("Timezone"),"</b>");
  gtk_label_set_markup (GTK_LABEL(self.catlabel4),fstr); 
  g_free(fstr);
  
  gtk_box_pack_start (GTK_BOX (self.catvbox4), self.catlabel4, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (self.catlabel4), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (self.catlabel4), 0, 0.5);

  self.catconthbox4 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (self.catvbox4), self.catconthbox4, TRUE, TRUE, 0);

  self.catindentlabel4 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (self.catconthbox4), self.catindentlabel4, FALSE, FALSE, 0);

  self.controlvbox4 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.catconthbox4), self.controlvbox4, TRUE, TRUE, 0);


  self.timezone = gtk_combo_new ();

  fstr = getenv("TZ");
  if (fstr) 
	fstr=g_strdup(getenv("TZ"));
  else
    fstr=g_strdup("UTC");

  for (idx=0; idx<3; idx++) {
    tzones = g_list_append (tzones, timezones[idx]);
    if (!strcmp(timezones[idx],fstr)) mark=idx;
  }

  gtk_combo_set_popdown_strings (GTK_COMBO (self.timezone), tzones);
  gtk_box_pack_start(GTK_BOX(self.controlvbox4), self.timezone, FALSE, FALSE, 0); 

  gtk_list_select_item(GTK_LIST(GTK_COMBO(self.timezone)->list),mark);
  
  g_free(fstr);
  /* -------------------------------------------------------------------------- */


  self.catvbox3 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.categories), self.catvbox3, TRUE, TRUE, 0);

  self.catlabel3 = gtk_label_new (NULL);
  fstr = g_strdup_printf("%s %s %s","<b>",_("Network"),"</b>");
  gtk_label_set_markup (GTK_LABEL(self.catlabel3),fstr); 
  g_free(fstr);
  
  gtk_box_pack_start (GTK_BOX (self.catvbox3), self.catlabel3, FALSE, FALSE, 0);
  gtk_label_set_justify (GTK_LABEL (self.catlabel3), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (self.catlabel3), 0, 0.5);

  self.catconthbox3 = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (self.catvbox3), self.catconthbox3, TRUE, TRUE, 0);

  self.catindentlabel3 = gtk_label_new (gpe_catindent);
  gtk_box_pack_start (GTK_BOX (self.catconthbox3), self.catindentlabel3, FALSE, FALSE, 0);

  self.controlvbox3 = gtk_vbox_new (FALSE, gpe_boxspacing);
  gtk_box_pack_start (GTK_BOX (self.catconthbox3), self.controlvbox3, TRUE, TRUE, 0);


  self.ntpserver = gtk_combo_new ();

  for (idx=0; idx<5; idx++) {
    ntpsrv = g_list_append (ntpsrv, Ntpservers[idx]);
  }

  gtk_combo_set_popdown_strings (GTK_COMBO (self.ntpserver), ntpsrv);
  gtk_box_pack_start(GTK_BOX(self.controlvbox3), self.ntpserver, FALSE, FALSE, 0);

  self.internet = gtk_button_new_with_label(_("Get time from network"));
  gtk_signal_connect (GTK_OBJECT(self.internet), "clicked",
		      (GtkSignalFunc) GetInternetTime, NULL);

  gtk_box_pack_start(GTK_BOX(self.controlvbox3), self.internet, FALSE, FALSE, 0);

  
   /*------------------------------*/

  
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
  char* par = malloc(100);
  
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

  /* set time */
  snprintf(par,99,"%ld",t);
  suid_exec("STIM",par);
  free(par);
  /* set timezone */
  if (setenv("TZ",gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(self.timezone)->entry)),1))
	  gpe_error_box(_("Could not set timezone."));
  suid_exec("STZO",gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(self.timezone)->entry)));
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
