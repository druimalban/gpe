#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>   
#include <sys/ioctl.h> 
#include <sys/time.h>
#include <sys/types.h>
#include <linux/h3600_ts.h>
#include <signal.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

extern GtkWidget *GPEBatMon;
gint UpdateInterval = 5000; /* msecs */
gint timeout_callback_tag;
/* Flag to show / not show the main menu bar,
   toggled with iPaq Task-Button, cool!			*/
static gboolean MainMenuShown = FALSE;

extern int TSfd;

gchar *ac_status[] = {
	"offline",
	"online",
	"backup"
};

gchar *bat_chemistries[] = {
	"",
	"Alkaline",
	"NiCd",
	"NiMh",
	"Lithium Ion",
	"Lithium Polymer"
};

gchar *bat_status[] = {
	"",
	"high",
	"low",
	"critical",
	"charging",
	"charge main",
	"dead",
	"full",
	"no battery"
};

gint update_bat_values(gpointer data)
{
static struct h3600_battery battery_val;
GtkWidget *WinVal;
float bperc;
gchar percstr[8];
gchar tmp[128];

	if (ioctl(TSfd, GET_BATTERY_STATUS, &battery_val) != 0)
		return 1;

	WinVal=lookup_widget(GPEBatMon,"tab2");
	if (battery_val.battery_count < 2) {
		gtk_widget_set_sensitive(WinVal, FALSE);
	} else
		gtk_widget_set_sensitive(WinVal, TRUE);

	WinVal=lookup_widget(GPEBatMon,"BatBar0");
	bperc=(float)battery_val.battery[0].percentage / (float)100;
	if (bperc > 1.0)
		bperc = 1.0;
	gtk_progress_bar_update(GTK_PROGRESS_BAR(WinVal),bperc);
	WinVal=lookup_widget(GPEBatMon,"Bat0Perc");
	sprintf(percstr,"%d%%",(unsigned char)battery_val.battery[0].percentage);
	gtk_label_set_text(GTK_LABEL(WinVal),percstr);

	WinVal=lookup_widget(GPEBatMon,"Bat0Chem");
	if ((battery_val.battery[0].chemistry > 0) && (battery_val.battery[0].chemistry <= 0x05))
		gtk_label_set_text(GTK_LABEL(WinVal),bat_chemistries[battery_val.battery[0].chemistry]);

	WinVal=lookup_widget(GPEBatMon,"Bat0Stat");
	switch (battery_val.battery[0].status) {
		case 0x01:
			gtk_label_set_text(GTK_LABEL(WinVal),"high");
			break;
		case 0x02:
			gtk_label_set_text(GTK_LABEL(WinVal),"low");
			break;
		case 0x04:
			gtk_label_set_text(GTK_LABEL(WinVal),"critical");
			break;
		case 0x08:
			gtk_label_set_text(GTK_LABEL(WinVal),"charging");
			break;
		case 0x10:
			gtk_label_set_text(GTK_LABEL(WinVal),"charge main");
			break;
		case 0x20:
			gtk_label_set_text(GTK_LABEL(WinVal),"dead");
			break;
		case 0x40:
			gtk_label_set_text(GTK_LABEL(WinVal),"full");
			break;
		case 0x80:
			gtk_label_set_text(GTK_LABEL(WinVal),"no battery");
			break;
		default:
			gtk_label_set_text(GTK_LABEL(WinVal),"unknown");
			break;
	};

	WinVal=lookup_widget(GPEBatMon,"Bat0Volt");
	sprintf(tmp,"%d mV",battery_val.battery[0].voltage * 5000 / 1024);
	gtk_label_set_text(GTK_LABEL(WinVal),tmp);

	WinVal=lookup_widget(GPEBatMon,"Bat0Life");
	sprintf(tmp,"%d min.",battery_val.battery[0].life);
	gtk_label_set_text(GTK_LABEL(WinVal),tmp);

	if (battery_val.battery_count > 1) {
		WinVal=lookup_widget(GPEBatMon,"BatBar1");
		bperc=(float)battery_val.battery[1].percentage / (float)100;
		if (bperc > 1.0)
			bperc = 1.0;
		gtk_progress_bar_update(GTK_PROGRESS_BAR(WinVal),bperc);
		WinVal=lookup_widget(GPEBatMon,"Bat1Perc");
		sprintf(percstr,"%d%%",(unsigned char)battery_val.battery[1].percentage);
		gtk_label_set_text(GTK_LABEL(WinVal),percstr);
	
		WinVal=lookup_widget(GPEBatMon,"Bat1Chem");
		if ((battery_val.battery[1].chemistry > 0) && (battery_val.battery[1].chemistry <= 0x05))
			gtk_label_set_text(GTK_LABEL(WinVal),bat_chemistries[battery_val.battery[1].chemistry]);
	
		WinVal=lookup_widget(GPEBatMon,"Bat1Stat");
		switch (battery_val.battery[1].status) {
			case 0x01:
				gtk_label_set_text(GTK_LABEL(WinVal),"high");
				break;
			case 0x02:
				gtk_label_set_text(GTK_LABEL(WinVal),"low");
				break;
			case 0x04:
				gtk_label_set_text(GTK_LABEL(WinVal),"critical");
				break;
			case 0x08:
				gtk_label_set_text(GTK_LABEL(WinVal),"charging");
				break;
			case 0x10:
				gtk_label_set_text(GTK_LABEL(WinVal),"charge main");
				break;
			case 0x20:
				gtk_label_set_text(GTK_LABEL(WinVal),"dead");
				break;
			case 0x40:
				gtk_label_set_text(GTK_LABEL(WinVal),"full");
				break;
			case 0x80:
				gtk_label_set_text(GTK_LABEL(WinVal),"no battery");
				break;
			default:
				gtk_label_set_text(GTK_LABEL(WinVal),"unknown");
				break;
		};

		WinVal=lookup_widget(GPEBatMon,"Bat1Volt");
		sprintf(tmp,"%d mV",battery_val.battery[1].voltage * 5000 / 1024);
		gtk_label_set_text(GTK_LABEL(WinVal),tmp);

		WinVal=lookup_widget(GPEBatMon,"Bat1Life");
		sprintf(tmp,"%d min.",battery_val.battery[1].life);
		gtk_label_set_text(GTK_LABEL(WinVal),tmp);
	} else {
		WinVal=lookup_widget(GPEBatMon,"BatBar1");
		gtk_progress_bar_update(GTK_PROGRESS_BAR(WinVal),0);
		WinVal=lookup_widget(GPEBatMon,"Bat1Perc");
		gtk_label_set_text(GTK_LABEL(WinVal),"unkn.");
	
		WinVal=lookup_widget(GPEBatMon,"Bat1Chem");
		gtk_label_set_text(GTK_LABEL(WinVal),"unkn.");
	
		WinVal=lookup_widget(GPEBatMon,"Bat1Stat");
		gtk_label_set_text(GTK_LABEL(WinVal),"unkn.");

		WinVal=lookup_widget(GPEBatMon,"Bat1Volt");
		gtk_label_set_text(GTK_LABEL(WinVal),"unkn.");

		WinVal=lookup_widget(GPEBatMon,"Bat1Life");
		gtk_label_set_text(GTK_LABEL(WinVal),"unkn.");
	}

return 1;
}


gboolean
on_GPEBatMon_de_event                  (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
	close(TSfd);
	gtk_main_quit();

  return FALSE;
}


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	close(TSfd);
	gtk_main_quit();
}


void
on_preferences1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *GPEBatMonPrefs;
GtkWidget *WUpdateInterval;

        GPEBatMonPrefs=create_GPEBatMonPrefs();

	WUpdateInterval=lookup_widget(GTK_WIDGET(GPEBatMonPrefs),"UpdateInterval");
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(WUpdateInterval), (float)UpdateInterval / (float)1000);

        gtk_window_set_transient_for(GTK_WINDOW(GPEBatMonPrefs),GTK_WINDOW(GPEBatMon));
        gtk_widget_show(GPEBatMonPrefs);
}


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
GtkWidget *GPEBatMonAbout;

        GPEBatMonAbout=create_GPEBatMonAbout();
        gtk_window_set_transient_for(GTK_WINDOW(GPEBatMonAbout),GTK_WINDOW(GPEBatMon));
        gtk_widget_show(GPEBatMonAbout);
}


void
on_AboutOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *GPEBatMonAbout;

	GPEBatMonAbout=lookup_widget(GTK_WIDGET(button),"GPEBatMonAbout");
	gtk_widget_hide(GPEBatMonAbout);
	gtk_widget_destroy(GPEBatMonAbout);
}


void
on_PrefsCancel_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *GPEBatMonPrefs;

	GPEBatMonPrefs=lookup_widget(GTK_WIDGET(button),"GPEBatMonPrefs");
	gtk_widget_hide(GPEBatMonPrefs);
	gtk_widget_destroy(GPEBatMonPrefs);
}


void
on_PrefsOK_clicked                     (GtkButton       *button,
                                        gpointer         user_data)
{
GtkWidget *GPEBatMonPrefs;
GtkWidget *WUpdateInterval;

	WUpdateInterval=lookup_widget(GTK_WIDGET(button),"UpdateInterval");
	UpdateInterval=1000 * gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(WUpdateInterval));
	gtk_timeout_remove(timeout_callback_tag);
	timeout_callback_tag=gtk_timeout_add(UpdateInterval,update_bat_values,NULL);

	GPEBatMonPrefs=lookup_widget(GTK_WIDGET(button),"GPEBatMonPrefs");
	gtk_widget_hide(GPEBatMonPrefs);
	gtk_widget_destroy(GPEBatMonPrefs);
}


gboolean
on_GPEBatMon_key_press_event           (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
	if (event->keyval == 0x1008ff1a) {
		GtkWidget *MainMenu;

		MainMenu=lookup_widget(GTK_WIDGET(widget),"MainMenu");
		if (MainMenuShown) {
			gtk_widget_hide(MainMenu);
			MainMenuShown = FALSE;
		} else {
			gtk_widget_show(MainMenu);
			MainMenuShown = TRUE;
		}
		return TRUE;
	}

return FALSE;
}


gboolean
on_GPEBatMon_button_press_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	if (event->button == 3) {
		GtkWidget *MainMenu;

		MainMenu=lookup_widget(GTK_WIDGET(widget),"MainMenu");
		if (MainMenuShown) {
			gtk_widget_hide(MainMenu);
			MainMenuShown = FALSE;
		} else {
			gtk_widget_show(MainMenu);
			MainMenuShown = TRUE;
		}
		return TRUE;
	}

return FALSE;
}


void
on_MainMenu_deactivate                 (GtkMenuShell    *menushell,
                                        gpointer         user_data)
{
GtkWidget *MainMenu;

	MainMenu=lookup_widget(GTK_WIDGET(menushell),"MainMenu");
	if (MainMenuShown) {
		gtk_widget_hide(MainMenu);
		MainMenuShown = FALSE;
	} else {
		gtk_widget_show(MainMenu);
		MainMenuShown = TRUE;
	}
}

