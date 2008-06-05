/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *               2003, 2004, 2008  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Module containing command line config tasks.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
#include "tasks.h"
#include "applets.h"
#include "sound/soundctrl.h"
#include "screen/brightness.h"

/* change primary nameserver in resolv.conf */
void 
task_change_nameserver(char *newns)
{
	if (!geteuid())
		change_cfg_value ("/etc/resolv.conf", "nameserver", newns, ' ');
	else
		fprintf(stderr,
		       "Only root is allowed to change the nameserver!\n");
	usleep(200000);
}

/* load/save sound settings */
void 
task_sound(char *action)
{
	if (!action) 
		return;
	if ((action[0] == 's') || !strcmp(action, "save"))
	{
		sound_init();
		sound_save_settings();
	}
	else if ((action[0] == 'r') || !strcmp(action, "restore"))
	{
		sound_init();
		
		if (get_mute_status())
			set_mute_status(TRUE);
	}
	else
		fprintf(stderr, "Invalid argument given\n");
}

void 
task_backlight(char *par1, char *par2)
{
	char buf[4];
	
	if (par1 == NULL && par2 == NULL) /* print current setting */
	{
		g_print("%s %d\n", backlight_get_power() ? "on" : "off", 
	             backlight_get_brightness());
		return;
	}
	if (par2) /* in this case: power, level */
	{
		snprintf (buf, 4, "%d", atoi (par2));
		suid_exec ("SCRP", strcasecmp (par1, "on") ? "0" : "1");
		suid_exec ("SCRB", buf);
		return;
	}
	
	/* default: just one setting */
	if (!strcasecmp(par1, "off")) 
		suid_exec ("SCRP", "0");
	else if (!strcasecmp(par1, "on")) 
		suid_exec ("SCRP", "1");
	else if (!strcasecmp(par1, "toggle"))
		suid_exec ("SCRP", !backlight_get_power() ? "1" : "0");
	else {
		suid_exec ("SCRP", "1");
		snprintf (buf, 4, "%d", atoi (par1));
		suid_exec ("SCRB", buf);
	}
}

void
task_shutdown(void)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, 
                                                         GTK_BUTTONS_YES_NO,
                                                         _("Are you sure you want to power down?\n" \
                                                           "Unsaved data from applications will be lost."));
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
		suid_exec ("SHDN", "1");
	gtk_widget_destroy (dialog);
}

