/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#include "interface.h"
#include "support.h"

GtkWidget *GPE_Time;

char do_update=1;

void alrmupdate(int sig)
{
GtkWidget *Calendar;
GtkWidget *SpinB;
struct timeval mytv;
struct tm *mytm;

	if (do_update) {
		Calendar=lookup_widget(GPE_Time, "Calendar");

		gettimeofday(&mytv, NULL);
		mytm=localtime(&mytv.tv_sec);
		gtk_calendar_select_month(GTK_CALENDAR(Calendar),mytm->tm_mon,(mytm->tm_year + 1900));
		gtk_calendar_select_day(GTK_CALENDAR(Calendar),mytm->tm_mday);

		SpinB=lookup_widget(GPE_Time, "Hour");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(SpinB),mytm->tm_hour);
		SpinB=lookup_widget(GPE_Time, "Minute");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(SpinB),mytm->tm_min);
		SpinB=lookup_widget(GPE_Time, "Second");
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(SpinB),mytm->tm_sec);
		alarm(1);
	}
}

int
main (int argc, char *argv[])
{
GtkWidget *SetB;

  gtk_set_locale ();
  gtk_init (&argc, &argv);
  
  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  /*
   * The following code was added by Glade to create one of each component
   * (except popup menus), just so that you see something after building
   * the project. Delete any components that you don't want shown initially.
   */
  GPE_Time = create_GPE_Time ();
  if (geteuid() != 0) {
  	SetB=lookup_widget(GPE_Time, "Set");
  	gtk_widget_set_sensitive(SetB,0);
  }

  alrmupdate(SIGALRM);
  do_update=0;
  signal(SIGALRM, alrmupdate);
  alarm(1);
  gtk_widget_show (GPE_Time);

  gtk_main ();
  return 0;
}
