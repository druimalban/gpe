#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdlib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/h3600_ts.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

static int ViewmodeVal=1;
static char AutoHide=0;

#define TS_DEV "/dev/h3600_ts"

FLITE_IN bl;

void
on_BacklightOn_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
int fd;

	fd = open(TS_DEV, O_RDWR);
	if (fd == -1)
		return;
	bl.pwr=1;
	ioctl(fd,FLITE_ON,(void *)&bl);
	close(fd);
	/*system("bl 1 1 1");*/
}


void
on_BacklightOff_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
int fd;

	fd = open(TS_DEV, O_RDWR);
	if (fd == -1)
		return;
	bl.pwr=0;
	ioctl(fd,FLITE_ON,(void *)&bl);
	close(fd);
	/*system("bl 1 0 1");*/
}


void
on_Calibrate_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	system("exec xcalibrate &");
}


void
on_viewmode_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	ViewmodeVal=(int) user_data;
}


void
on_ViewmodeApply_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
	switch (ViewmodeVal) {
	case 1:
		system("xrandr -o normal");
		system("xmodmap /etc/X11/xmodmap-portrait");
		break;
	case 2:
		system("xrandr -o right");
		system("xmodmap /etc/X11/xmodmap-right");
		break;
	case 3:
		system("xrandr -o inverted");
		system("xmodmap /etc/X11/xmodmap-invert");
		break;
	case 4:
		system("xrandr -o left");
		system("xmodmap /etc/X11/xmodmap-left");
		break;
	}
}


void on_LightLevel_changed  (GtkAdjustment *adj, gpointer user_data)
{
int fd;

	fd = open(TS_DEV, O_RDWR);
	if (fd == -1)
		return;
	bl.brightness=(unsigned char)adj->value;
	ioctl(fd,FLITE_ON,(void *)&bl);
	close(fd);
}

void
on_AutoHideOn_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if (AutoHide == 0) {
		AutoHide=1;
		system("kill `ps -C unclutter --noheading -o %p` > /dev/null 2>&1");
		system("exec unclutter -idle 1 -root -visible &");
	}
}


void
on_AutoHideOff_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	if (AutoHide == 1) {
		AutoHide=0;
		system("kill `ps -C unclutter --noheading -o %p` > /dev/null 2>&1");
	}
}


void
on_XsetOn_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
GtkWidget *spinb;
unsigned int timeout, cycle;
char cmd[64];

	spinb=lookup_widget(GTK_WIDGET(togglebutton),"XsetTimeout");
	timeout=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinb));
	spinb=lookup_widget(GTK_WIDGET(togglebutton),"XsetCycle");
	cycle=gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spinb));
	sprintf(cmd,"xset s %d %d",timeout,cycle);
	system(cmd);
}


void
on_XsetOff_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
	system("xset s off");
}

