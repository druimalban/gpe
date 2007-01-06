#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

extern char do_update;

void
on_Set_clicked                         (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *Calendar;
GtkWidget *SpinB;
struct timeval mytv;
struct tm mytm;
time_t mytt;

	Calendar=lookup_widget(GTK_WIDGET(button), "Calendar");

	gtk_calendar_get_date(GTK_CALENDAR(Calendar),&mytm.tm_year,&mytm.tm_mon,&mytm.tm_mday);
	mytm.tm_year -= 1900;

	SpinB=lookup_widget(GTK_WIDGET(button), "Hour");
	mytm.tm_hour=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SpinB));
	SpinB=lookup_widget(GTK_WIDGET(button), "Minute");
	mytm.tm_min=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SpinB));
	SpinB=lookup_widget(GTK_WIDGET(button), "Second");
	mytm.tm_sec=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(SpinB));

	mytt=mktime(&mytm);
	fprintf(stderr,"to set time: %s\n",ctime(&mytt));
	mytv.tv_sec=mytt;
	mytv.tv_usec=0;
	if (settimeofday(&mytv,NULL) != 0) {
		fprintf(stderr,"error setting time\n");
		perror("settimeofday");
	}
}


void
on_Cancel_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_main_quit();
}


void
on_Update_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if (do_update) {
		do_update=0;
	} else {
		do_update=1;
		alarm(1);
	}
}

