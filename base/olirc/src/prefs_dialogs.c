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
#include "prefs.h"
#include "windows.h"

gchar pdtmp[1024];

struct w_pref
{
	GtkWidget *master_widget;
	GtkWidget *slave_widget;
};

struct w_pref w_prefs[PT_LAST];

/* ---------- Useful functions ---------- */

void prefs_choose_font_destroy(GtkWidget *w, gpointer data)
{
	OlircPrefType font_type = (OlircPrefType) data;
	w_prefs[font_type].slave_widget = NULL;
}

void prefs_choose_font_ok(GtkWidget *w, gpointer data)
{
	OlircPrefType font_type = (OlircPrefType) data;
	gchar *name;

	name = gtk_font_selection_dialog_get_font_name((GtkFontSelectionDialog *) w_prefs[font_type].slave_widget);

	if (name)
	{
		gtk_entry_set_text((GtkEntry *) w_prefs[font_type].master_widget, name);
		g_free(name);
	}

	gtk_widget_destroy(w_prefs[font_type].slave_widget);
}

void prefs_choose_font(GtkWidget *w, gpointer data)
{
	GtkWidget *fs;
	OlircPrefType font_type = (OlircPrefType) data;
	struct o_sub_pref *osp;

	if (w_prefs[font_type].slave_widget)
	{
		gdk_window_raise(w_prefs[font_type].slave_widget->window);
		return;
	}

	sprintf(pdtmp, "Please choose the %s", o_prefs[font_type].label);
	fs = gtk_font_selection_dialog_new(pdtmp);

	osp = (struct o_sub_pref *) o_prefs[font_type].sub_prefs->data;

	gtk_font_selection_dialog_set_preview_text((GtkFontSelectionDialog *) fs, "Ol-Irc " VER_STRING " by Olrick");
	gtk_font_selection_dialog_set_font_name((GtkFontSelectionDialog *) fs, (const gchar *) osp->value);

	gtk_signal_connect((GtkObject *) fs, "key_press_event", (GtkSignalFunc) dialog_simple_key_press_event, (gpointer) fs);
	gtk_signal_connect((GtkObject *) fs, "destroy", (GtkSignalFunc) prefs_choose_font_destroy, (gpointer) font_type);
	gtk_signal_connect_object((GtkObject *) ((GtkFontSelectionDialog *) fs)->cancel_button, "clicked", (GtkSignalFunc) gtk_widget_destroy, (GtkObject *) fs);
	gtk_signal_connect((GtkObject *) ((GtkFontSelectionDialog *) fs)->ok_button, "clicked", (GtkSignalFunc) prefs_choose_font_ok, (gpointer) font_type);

	gtk_widget_show(fs);

	w_prefs[font_type].slave_widget = fs;
}

void prefs_add_font(GtkWidget *table, gint x, gint y, OlircPrefType font_type)
{
	GtkWidget *plus, *wlabel, *choose;
	struct o_sub_pref *osp;

	g_return_if_fail(o_prefs[font_type].sub_prefs);

	osp = (struct o_sub_pref *) o_prefs[font_type].sub_prefs->data;

	plus = gtk_button_new_with_label(" + ");
	gtk_widget_set_sensitive(plus, FALSE);
	gtk_table_attach((GtkTable *) table, plus, x, x+1, y, y+1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_EXPAND), 10, 8);
	x++;

	wlabel = gtk_label_new(o_prefs[font_type].label);
	gtk_table_attach((GtkTable *) table, wlabel, x, x+1, y, y+1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_EXPAND), 10, 8);
	x++;

	w_prefs[font_type].master_widget = gtk_entry_new_with_max_length(256);
	gtk_table_attach((GtkTable *) table, w_prefs[font_type].master_widget, x, x+1, y, y+1, (GtkAttachOptions) (GTK_FILL | GTK_EXPAND), (GtkAttachOptions) (GTK_EXPAND), 10, 8);
	x++;

	if (osp->value) gtk_entry_set_text((GtkEntry *) w_prefs[font_type].master_widget, (gchar *) osp->value);

	choose = gtk_button_new_with_label(" Choose ");
	GTK_WIDGET_UNSET_FLAGS(choose, GTK_CAN_FOCUS);
	gtk_table_attach((GtkTable *) table, choose, x, x+1, y, y+1, (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_EXPAND), 10, 8);
	gtk_signal_connect((GtkObject *) choose, "clicked", (GtkSignalFunc) prefs_choose_font, (gpointer) font_type);
}

/* ---------- Notebook pages initialization ---------- */

void prefs_no_init(GtkWidget *box)
{
	GtkWidget *label = gtk_label_new("Not done yet, sorry...");
	gtk_box_pack_start((GtkBox *) box, label, TRUE, TRUE, 10);
}

void prefs_no_save()
{
	/* Nothing */
}

OlircPrefType font_types[] = { PT_FONT_ENTRY, PT_FONT_NORMAL, PT_FONT_BOLD, PT_FONT_UNDERLINE, PT_FONT_BOLD_UNDERLINE };

void page_fonts_init(GtkWidget *box)
{
	GtkWidget *table;
	gint i;

	table = gtk_table_new(5, 5, FALSE);
	gtk_box_pack_start((GtkBox *) box, table, TRUE, TRUE, 10);

	for (i=0; i < sizeof(font_types)/sizeof(gint); i++) prefs_add_font(table, 0, i, font_types[i]);
}

void page_fonts_save()
{
	gint i;

	for (i=0; i < sizeof(font_types)/sizeof(gint); i++)
		prefs_set_value(font_types[i], NULL, g_strdup(gtk_entry_get_text((GtkEntry *) w_prefs[font_types[i]].master_widget)));
}

/* ---------- Structures ---------- */

#define MAX_PAGES 16

struct
{
	gchar *title;
	void (* init_func)(GtkWidget *);
	void (* save_func)();
} prefs_pages[] =
{
	{ "General", prefs_no_init, prefs_no_save },
	{ "Colors", prefs_no_init, prefs_no_save },
	{ "Fonts", page_fonts_init, page_fonts_save },
	{ "Windows", prefs_no_init, prefs_no_save },
	{ "Servers", prefs_no_init, prefs_no_save },
	{ "Channels", prefs_no_init, prefs_no_save },
	{ "CTCP", prefs_no_init },
	{ NULL, NULL }
};

struct d_prefs_box
{
	struct dbox dbox;
	GtkWidget *page[MAX_PAGES];
};

struct d_prefs_box *dpb = NULL;

/* ---------- Global management of the prefs dialog box ---------- */

void dp_destroy(GtkWidget *w, gpointer data)
{
	gint i;

	for (i=0; i < PT_LAST; i++)
		if (w_prefs[i].slave_widget) gtk_widget_destroy(w_prefs[i].slave_widget);

	g_free_and_NULL(dpb);
}

void dp_prefs_button_apply(GtkWidget *w, gpointer data)
{
	gint i = 0;
	while (prefs_pages[i].save_func) { prefs_pages[i].save_func(); i++; }
	VW_reload_prefs();
}

void dp_prefs_button_ok(GtkWidget *w, gpointer data)
{
	dp_prefs_button_apply(w, data);
	gtk_widget_destroy(dpb->dbox.window);
}

void dialog_prefs()
{
	GtkWidget *mvbox, *vbox, *nb;
	GtkWidget *btmp, *wtmp, *ftmp;
	gint i;

	if (dpb)
	{
		gdk_window_raise(dpb->dbox.window->window);
		return;
	}

	dpb = (struct d_prefs_box *) g_malloc0(sizeof(struct d_prefs_box));

	dpb->dbox.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_position((GtkWindow *) dpb->dbox.window, GTK_WIN_POS_CENTER);
	gtk_signal_connect((GtkObject *) dpb->dbox.window, "destroy", (GtkSignalFunc) dp_destroy, (gpointer) NULL);
	gtk_signal_connect((GtkObject *) dpb->dbox.window, "key_press_event", (GtkSignalFunc) dialog_key_press_event, (gpointer) dpb);
	gtk_window_set_policy((GtkWindow *) dpb->dbox.window, TRUE, TRUE, FALSE);
	gtk_window_set_title((GtkWindow *) dpb->dbox.window, "Ol-Irc " VER_STRING " Preferences");

	mvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) dpb->dbox.window, mvbox);

	ftmp = gtk_frame_new(NULL);
	gtk_container_border_width((GtkContainer *) ftmp, 2);
	gtk_box_pack_start((GtkBox *) mvbox, ftmp, TRUE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add((GtkContainer *) ftmp, vbox);
	gtk_container_border_width((GtkContainer *) vbox, 4);

	nb = gtk_notebook_new();
	GTK_WIDGET_UNSET_FLAGS(nb, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) vbox, nb, TRUE, TRUE, 0);

	gtk_widget_set_usize(nb, 700, 300);

	i = 0;
	while (prefs_pages[i].title)
	{
		wtmp = gtk_label_new(prefs_pages[i].title);
		dpb->page[i] = gtk_vbox_new(FALSE, 0);
		gtk_notebook_append_page((GtkNotebook *) nb, dpb->page[i], wtmp);
		prefs_pages[i].init_func(dpb->page[i]);
		i++;
	}

	btmp = gtk_hbox_new(TRUE, 10);
	gtk_container_border_width((GtkContainer *) btmp, 5);
	gtk_box_pack_start((GtkBox *) mvbox, btmp, FALSE, FALSE, 0);

	wtmp = gtk_button_new_with_label("Ok");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 5);
	gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dp_prefs_button_ok, (gpointer) dpb);

	wtmp = gtk_button_new_with_label("Apply");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 5);
	gtk_signal_connect((GtkObject *) wtmp, "clicked", (GtkSignalFunc) dp_prefs_button_apply, (gpointer) dpb);

	wtmp = gtk_button_new_with_label("Cancel");
	GTK_WIDGET_UNSET_FLAGS(wtmp, GTK_CAN_FOCUS);
	gtk_box_pack_start((GtkBox *) btmp, wtmp, TRUE, TRUE, 5);
	gtk_signal_connect_object((GtkObject *) wtmp, "clicked", (GtkSignalFunc) gtk_widget_destroy, (gpointer) dpb->dbox.window);

	gtk_widget_show_all(dpb->dbox.window);

	gtk_notebook_set_page((GtkNotebook *) nb, 2);
}

/* vi: set ts=3: */

