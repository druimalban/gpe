/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "olirc.h"
#include "windows.h"
#include "prefs.h"
#include "menus.h"
#include "servers.h"
#include "dialogs.h"
#include "scripting.h"
#include "console.h"
#include "entry.h"

#include <unistd.h>
#include <signal.h>
#include <errno.h>

struct Root_Object *Olirc;

void SIG_Handler(int n)
{
	if (n == SIGTERM) g_print("--> Caught SIGTERM\n");
	else if (n == SIGINT) g_print("--> Caught SIGINT\n");
	else if (n == SIGPIPE) { signal(SIGPIPE, SIG_Handler); return; }
	else if (n == SIGUSR2) abort();
	else g_print("--> Caught signal %d\n",n);
	gtk_exit(1);
}

int main(int argc, char *argv[])
{
	GdkColormap *cmap;
	Console *n;

	gint i;

	#ifdef DEBUG
	fprintf(stdout, "\n--- Debugging mode enabled ---\n\n");
	#endif

	/* Root object initialization */

	Olirc = (struct Root_Object *) g_malloc0(sizeof(struct Root_Object));
	Olirc->Flags |= FLAG_STARTING;

	/* PreParse the command line */

	PreParse_Command_Line(argc, argv);

	/* Prefs initialization */

	Prefs_Init(argc, argv);

	/* If we must fork(), do it now */

	if (Olirc->Flags & FLAG_FORK)
	{
		gint r = fork();

		if (r==-1)
		{
			fprintf(stderr, "\n\nUnable to fork(): %s ! Giving up.\n\n", g_strerror(errno));
			exit(1);
		}
		else if (r) exit(0);
	
		#ifdef DEBUG
		fprintf(stdout, "\n\nfork()ed. new PID = %d\n\n", (int) getpid());
		#endif
	}

	/* Gtk initialization */

	gtk_init(&argc, &argv);

	/* PostParse the command line */

	PostParse_Command_Line(argc, argv);

	/* Some signals handlers */

	signal(SIGTERM, SIG_Handler);
	signal(SIGINT,  SIG_Handler);
	signal(SIGPIPE, SIG_Handler);
	signal(SIGUSR2, SIG_Handler);

	/* Colors initialization */

	if (gdk_visual_get_system()->depth < 2) Olirc->Flags |= FLAG_MONOCHROME;

	cmap = gdk_colormap_get_system();

	if (Olirc->Flags & FLAG_MONOCHROME)
	{
		Olirc->Colors[0].red = 255<<8; Olirc->Colors[0].green = 255<<8; Olirc->Colors[0].blue = 255<<8;
		gdk_color_alloc(cmap, &Olirc->Colors[0]);
		for (i=1; i<16; i++)
		{
			Olirc->Colors[i].red = 0; Olirc->Colors[i].green = 0; Olirc->Colors[i].blue = 0;
			gdk_color_alloc(cmap, &Olirc->Colors[i]);
		}
	}
	else for (i=0; i<16; i++)
	{
		Olirc->Colors[i].red = (GPrefs.Colors[i]>>8) & 0xFF00;
		Olirc->Colors[i].green = GPrefs.Colors[i] & 0xFF00;
		Olirc->Colors[i].blue = (GPrefs.Colors[i]<<8) & 0xFF00;

		gdk_color_alloc(cmap, &Olirc->Colors[i]);
	}

	/* Menus initialization */

	Popup_Menus_Init();

	/* Random generator initialization (used in PING requests...) */

	srand(time(NULL));

	/* Initalize the entry commands */

	init_entry_parser_commands();

	/* Creation and setup of the main window (the console) */

	n = Console_New(NULL);

	/* Display a few copyright infos */

	Console_Copyright(n);

	/* Initialize the scripting system */

	Script_Init(argc, argv, n->vw);

	/* Reopen servers and channels */

	Servers_Reopen();

	/* Activate the console */

	Olirc->Flags &= ~FLAG_STARTING;

	VW_Activate(n->vw);

	if (getuid()==0) Message_Box("-- Warning --", "!!!!! Please DO NOT run Ol-Irc as root !!!!!");

	/* Start the timer function */

	Olirc->main_timer_tag = gtk_timeout_add(1000, (GtkFunction) olirc_timer, NULL);

	/* Enter the Gtk main loop */

	gtk_main();

	return 0;
}

/* vi: set ts=3: */

