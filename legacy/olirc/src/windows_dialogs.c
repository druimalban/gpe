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
#include "dialogs.h"
#include "windows_dialogs.h"
#include "windows.h"
#include "misc.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <strings.h>
#include <errno.h>

gchar wd_tmp[4096];

/* ----- Window log dialog box ---------------------------------------------------- */

void VW_Log_Ok(GtkWidget *w, gpointer data)
{
	GtkWidget *filesel = (GtkWidget *) data;
	Virtual_Window *vw = (Virtual_Window *) gtk_object_get_data(GTK_OBJECT(filesel), "VW");
	gchar *file = g_strdup(Remember_Filepath((GtkFileSelection *) filesel, &Olirc->logfiles_path));
	struct stat buf;
	gint fd = -1;

	if (stat(file, &buf) == -1) /* We must create the file */
	{
		gtk_widget_destroy(filesel);

		fd = creat(file, 0600);
		if (fd==-1)
		{
			sprintf(wd_tmp, "Error while creating logfile.\n(%s)", g_strerror(errno));
			Message_Box("Error", wd_tmp);
		}

	}
	else if (S_ISREG(buf.st_mode)) /* We must append to an existing file */
	{
		gtk_widget_destroy(filesel);

		fd = open(file, O_RDWR | O_APPEND);
		if (fd==-1)
		{
			sprintf(wd_tmp, "Error while opening logfile.\n(%s)", g_strerror(errno));
			Message_Box("Error", wd_tmp);
		}
	}
	else
	{
		sprintf(wd_tmp, "%s is not a regular file.", file);
		Message_Box("Error", wd_tmp);
	}

	if (fd != -1)
	{
		gchar *txt;
		static gchar *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
		static gchar *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
		time_t tt = time(NULL);
		struct tm *t = localtime(&tt);
		VW_output(vw, T_WARNING, "F--", "Started log of %s in file %s", vw->Name, file);
		vw->fd_log = fd;
		sprintf(wd_tmp,"\n\nLog of %s, on %s, %s %d %d.\n\n", vw->Name, days[t->tm_wday], months[t->tm_mon], t->tm_mday, t->tm_year+1900);
		write(vw->fd_log, wd_tmp, strlen(wd_tmp));
		txt = gtk_editable_get_chars(GTK_EDITABLE(vw->Text), 0, -1);
		write(vw->fd_log, txt, strlen(txt));
		write(vw->fd_log, "\n", 1);
		g_free(txt);
	}

	g_free(file);
}

void VW_Log(Virtual_Window *vw)
{
	GtkWidget *filesel;

	if (vw->fd_log != -1)
	{
		VW_output(vw, T_NORMAL, "t", "Log stopped.");
		close(vw->fd_log); vw->fd_log = -1;
		return;
	}

	sprintf(wd_tmp, "Select a logfile for %s", vw->Name);
	filesel = gtk_file_selection_new(wd_tmp);

	gtk_object_set_data(GTK_OBJECT(filesel), "VW", (gpointer) vw);

	if (Olirc->logfiles_path)
		gtk_file_selection_set_filename((GtkFileSelection *) filesel, Olirc->logfiles_path);

	sprintf(wd_tmp, "%s.log", vw->Name);
	gtk_file_selection_set_filename((GtkFileSelection *) filesel, wd_tmp);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->ok_button), "clicked", GTK_SIGNAL_FUNC(VW_Log_Ok), (gpointer) filesel);
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(filesel)->cancel_button), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) filesel);

	Add_Home_Button((GtkFileSelection *) filesel);

	{
		GtkWidget *wtmp;
		wtmp = gtk_check_button_new_with_label("Continue to log in this file every time the window is open");
		gtk_widget_set_sensitive(wtmp, FALSE);
		GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
		gtk_box_pack_end((GtkBox *) ((GtkFileSelection *) filesel)->main_vbox, wtmp, FALSE, FALSE, 0);
		gtk_widget_show(wtmp);
	}

	gtk_widget_show(filesel);
}

/* ----- GUI Window selection box ----------------------------------------------------- */

#include <gdk/gdkkeysyms.h>

struct dsgw_box
{
	struct dbox dbox;

	GtkWidget *create_button;
	GtkWidget *move_button;
};

struct dsgw_box *sgw_box = NULL;

GUI_Window *sgw_gw_selected = NULL;
Virtual_Window *sgw_source_vw = NULL;

void dsrw_destroy(GtkWidget *widget, gpointer data)
{
	sgw_box = NULL;
}

void dsrw_move(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(sgw_box->dbox.window);
	VW_Move_To_GW(sgw_source_vw, sgw_gw_selected);
}

void dsrw_create(GtkWidget *widget, gpointer data)
{
	gtk_widget_destroy(sgw_box->dbox.window);
	VW_Move_To_GW(sgw_source_vw, NULL);
}

void srw_row_selected(GtkWidget *widget, gint row, gint column, GdkEventButton *event)
{
	sgw_gw_selected = (GUI_Window *) gtk_clist_get_row_data(GTK_CLIST(widget), row);
	gtk_widget_set_sensitive(sgw_box->move_button, TRUE);
}

void srw_row_unselected(GtkWidget *widget, gint row, gint column, GdkEventButton *event)
{
	sgw_gw_selected = NULL;
	gtk_widget_set_sensitive(sgw_box->move_button, TRUE);
}

void dialog_rw_select(Virtual_Window *vw)
{
	gint k,n;
	gchar *t;
	GList *r, *v;
	GUI_Window *rw;
	gchar *Title[] = { "Windows" };
	GtkWidget *cl, *ftmp, *vbox, *btmp, *wtmp;
	
	sgw_box = (struct dsgw_box *) g_malloc0(sizeof(struct dsgw_box));

	sgw_box->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_position((GtkWindow *) sgw_box->dbox.window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) sgw_box->dbox.window, "destroy", (GtkSignalFunc) dsrw_destroy, NULL);
	gtk_signal_connect((GtkObject *) sgw_box->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) sgw_box);
	gtk_window_set_title((GtkWindow *) sgw_box->dbox.window, "Window selection");

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) sgw_box->dbox.window, vbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) vbox, ftmp, TRUE, TRUE, 0);

	btmp = gtk_vbox_new(FALSE, 5);
	gtk_container_border_width((GtkContainer *) btmp, 6);
   gtk_container_add((GtkContainer *) ftmp, btmp);

	if (vw->rw->vw_list->next)
		sprintf(wd_tmp, "You can move '%s' into one of these windows, or create a new window", vw->Name);
	else
		sprintf(wd_tmp, "You can move '%s' into one of these windows", vw->Name);

	wtmp = gtk_label_new(wd_tmp);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, FALSE, FALSE, 0);

	cl = gtk_clist_new_with_titles(1,Title);

	GTK_WIDGET_UNSET_FLAGS(cl, GTK_CAN_FOCUS);
	gtk_clist_column_titles_passive((GtkCList *) cl);
	gtk_clist_set_selection_mode((GtkCList *) cl, GTK_SELECTION_SINGLE);

	gtk_signal_connect((GtkObject *) cl, "select_row", (GtkSignalFunc) srw_row_selected, NULL);
	gtk_signal_connect((GtkObject *) cl, "unselect_row", (GtkSignalFunc) srw_row_unselected, NULL);

	sgw_source_vw = vw;

	t = wd_tmp;
	n = 0;

	r = GW_List;
	while (r)
	{
		rw = r->data;
		if (vw->rw != rw)
		{
			sprintf(wd_tmp," %d ", ++n);
			v = rw->vw_list;
			while (v)
			{
				strspacecat(wd_tmp, ((Virtual_Window *) v->data)->Name);
				v = v->next;
			}
			k = gtk_clist_append((GtkCList *) cl, &t);
			gtk_clist_set_row_data((GtkCList *) cl, k, rw);
		}

		r = r->next;
	}

	gtk_widget_set_usize(cl, 0, 250);

	{
		wtmp = gtk_scrolled_window_new(NULL, NULL);
		gtk_container_add((GtkContainer *) wtmp, cl);
		gtk_scrolled_window_set_policy((GtkScrolledWindow *) wtmp, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);
	}

	btmp = gtk_hbox_new(TRUE, 10);
	gtk_container_border_width((GtkContainer *) btmp, 10);
	gtk_box_pack_start((GtkBox *) vbox, btmp, FALSE, FALSE, 0);

	if (sgw_source_vw->rw->vw_list->next)
	{
		sgw_box->create_button = gtk_button_new_with_label(" Create a new window ");
		GTK_WIDGET_UNSET_FLAGS(sgw_box->create_button, GTK_CAN_FOCUS);
		gtk_box_pack_start((GtkBox *) btmp, sgw_box->create_button, TRUE, TRUE, 0);
		gtk_signal_connect((GtkObject *) sgw_box->create_button, "clicked", (GtkSignalFunc) dsrw_create, NULL);
	}

	sgw_box->move_button = gtk_button_new_with_label(" Move into this window ");
	GTK_WIDGET_UNSET_FLAGS(sgw_box->move_button, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, sgw_box->move_button, TRUE, TRUE, 0);
	gtk_widget_set_sensitive(sgw_box->move_button, FALSE);
	gtk_signal_connect((GtkObject *) sgw_box->move_button, "clicked", (GtkSignalFunc) dsrw_move, NULL);

	wtmp = gtk_button_new_with_label(" Cancel ");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 0);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (GtkObject *) sgw_box->dbox.window);

	gtk_widget_show_all(sgw_box->dbox.window);
	gtk_grab_add(sgw_box->dbox.window);
}

/* vi: set ts=3: */

