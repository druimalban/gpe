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

#include <string.h>

#include "olirc.h"
#include "console.h"
#include "windows.h"
#include "prefs.h"
#include "histories.h"

#ifdef USE_TCL

#include "tcl-olirc.h"

#endif

gchar cons_tmp[1024];

void Console_Copyright(Console *n)
{
	g_return_if_fail(n);
	g_return_if_fail(n->vw);

	VW_output(n->vw, T_NORMAL, "t", "\n\0032 Welcome to Ol-Irc " VER_STRING " ! \n");
	VW_output(n->vw, T_NORMAL, "t", "\00310 Ol-Irc is copyright (C) 1998, 1999 Yann Grossel [Olrick].\n");

	VW_output(n->vw, T_NORMAL, "t", "\00314 Ol-Irc is released under the \0035GNU Public License\00314.\n"
	" This is free software, and you are welcome to redistribute it under certain conditions.\n"
	" It comes with \0035ABSOLUTELY NO WARRANTY\00314. For more details, please see the file COPYING,\n"
	" or type olirc --license.\n");
}

void Console_Input(gpointer data, gchar *text)
{
	Console *n = (Console *) data;

	#ifdef USE_TCL

	tcl_do(n->vw, text);

	#else

	VW_output(n->vw, T_WARNING, "t", "You can't send any text to this window.");

	#endif
}

void Console_Close(gpointer data)
{
	Console *n = (Console *) data;

	if (n->vw->rw->vw_active == n->vw) GW_Activate_Last_VW(n->vw->rw);

	VW_Remove_From_GW(n->vw, FALSE);
	g_free(n);
}

void Console_Infos(gpointer data)
{
	Console *n = (Console *) data;

	strcpy(cons_tmp ,"Ol-Irc " VER_STRING);
	gtk_label_set((GtkLabel *) n->vw->WFL[0], cons_tmp);

	strcpy(cons_tmp , RELEASE_DATE);
	gtk_label_set((GtkLabel *) n->vw->WFL[1], cons_tmp);
}

Console *Console_New(GUI_Window *rw)
{
	Console *n;

	n = (Console *) g_malloc0(sizeof(struct Console));

	n->vw = VW_new();
	n->vw->pmask.w_type = W_CONSOLE;
	n->vw->Name = "Console";
	n->vw->Title = "Ol-Irc " VER_STRING;
	n->vw->Resource = (gpointer) n;
	n->vw->Infos = Console_Infos;
	n->vw->Close = Console_Close;
	n->vw->Input = Console_Input;

	VW_Init(n->vw, rw, 2);

	gtk_widget_show_all(n->vw->Page_Box);

	return n;
}

/* vi: set ts=3: */

