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

#ifndef __OLIRC_DIALOGS_H__
#define __OLIRC_DIALOGS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct dbox
{
	gboolean stop_focus;

	void  (* return_func) (gpointer);
	void  (* suppr_func) (gpointer);

	GtkWidget *window;
};

gint dialog_simple_key_press_event(GtkWidget *, GdkEventKey *, gpointer);
gint dialog_key_press_event(GtkWidget *, GdkEventKey *, gpointer);

extern GtkWidget *Pack_Button(GtkDialog *, gchar *, void (GtkWidget *, gint), gint);
extern void Add_Dialog_Table_Button(GtkWidget *, gchar *, void (GtkWidget *, gint), gint, gint, gint);
extern GtkWidget *Add_Dialog_Table_Entry(GtkWidget *, gchar *, gint, gint);
extern void Message_Box(gchar *, gchar *);

extern void Dialog_Socks();

void Add_Home_Button(GtkFileSelection *);

void olirc_real_quit();
void olirc_quit(gchar *, gboolean);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OLIRC_DIALOGS_H_ */

/* vi: set ts=3: */

