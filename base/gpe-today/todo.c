/*
 * Copyright (C) 2002 Luis 'spung' Oliveira <luis@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gpe/errorbox.h>
#include <gpe/todo-db.h>
#include "today.h"

#define TODO_DB "/.gpe/todo"
#define UPDATE_INTERVAL 60000 /* 1 minute */

static char *db_fname;
static gboolean db_opened = FALSE;
static int todo_update_tag = 0;

static GSList *items;

struct display_item {
	struct todo_item *ti;
	PangoLayout *pl;
};

static void free_items(void)
{
	int i;
	struct display_item *di;

	for (i=0; (di = g_slist_nth_data(items, i)); i++) {
		g_free(di->ti);
		g_object_unref(di->pl);
	}

	g_slist_free(todo.scroll->list);
	g_slist_free(items);
	items = todo.scroll->list = NULL;
}

static void refresh_todo_widget(void)
{
        myscroll_update_upper_adjust(todo.scroll);

        if (todo.scroll->adjust->page_size >= todo.scroll->adjust->upper)
                gtk_widget_hide(todo.scroll->scrollbar);
        else if (!GTK_WIDGET_VISIBLE(todo.scroll->scrollbar))
                gtk_widget_show(todo.scroll->scrollbar);

        gtk_widget_queue_draw(todo.scroll->draw);
        gtk_widget_queue_draw(todo.toplevel);
}

void todo_init(void)
{
	GtkWidget *vboxlogo, *logo;
	GdkPixmap *pix;
	GdkBitmap *mask;
	gchar *home = (gchar *) g_get_home_dir();

	todo.message = NULL;

	/* todo db full path */
	db_fname = g_malloc(strlen(home) + strlen(TODO_DB) + 1);
	strcpy(db_fname, home);
	strcat(db_fname, TODO_DB);

	todo.toplevel = gtk_hbox_new(FALSE, 0);

	/* todo icon */
	vboxlogo = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(todo.toplevel), vboxlogo, FALSE, FALSE, 5);

	load_pixmap(IMAGEPATH(tasks.png), &pix, &mask, 130);
	logo = gtk_pixmap_new(pix, mask);
	gtk_box_pack_start(GTK_BOX(vboxlogo), logo, FALSE, FALSE, 0);

	/* scrollable pango layout list */
	todo.scroll = myscroll_new(TRUE);
	gtk_box_pack_start(GTK_BOX(todo.toplevel), todo.scroll->draw, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(todo.toplevel), todo.scroll->scrollbar,
			   FALSE, FALSE, 0);

	gtk_widget_show_all(todo.toplevel);

	/* update and set regular update intervals */
	todo_update(NULL);
	todo_update_tag = g_timeout_add(UPDATE_INTERVAL, todo_update, NULL);
}

void todo_free(void)
{
	free_items();
//	gtk_container_remove(GTK_CONTAINER(window.vbox1), todo.toplevel);
	g_free(db_fname);
	g_source_remove(todo_update_tag);

	if (db_opened)
		todo_db_stop();
}

static void todo_db_update(void)
{
	GSList *iter;
	struct display_item *di;

	if (db_opened)
		todo_db_stop();

	db_opened = TRUE;
	if (todo_db_start())
		return;

	free_items();

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		di = g_malloc(sizeof(struct display_item));
		g_slist_append(items, di);
		di->ti = iter->data;

		di->pl = gtk_widget_create_pango_layout(todo.scroll->draw, NULL);
		pango_layout_set_wrap(di->pl, PANGO_WRAP_WORD);
		todo.scroll->list = g_slist_append(todo.scroll->list, di->pl);
		markup_printf(di->pl, "%s", di->ti->summary);
	}
}

gboolean todo_update(gpointer data)
{
	struct stat db;
	static time_t prev_modified = 0;

	if (stat(db_fname, &db) == -1) {
		gpe_perror_box(_("Error stating todo DB"));
		return FALSE;
	}

	if (db.st_mtime > prev_modified) {
		prev_modified = db.st_mtime;
		todo_db_update();
	}

	refresh_todo_widget();

	return TRUE;
}
