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
	gboolean overdue;
};

static void refresh_todo_markup(struct display_item *di);

static void make_no_items_label(void)
{
	const char *str = _("No todo items");
	
	if (todo.message)
		return;
	
	todo.message = gtk_widget_create_pango_layout(todo.scroll->draw, NULL);
	pango_layout_set_wrap(todo.message, PANGO_WRAP_WORD);
	todo.scroll->list = g_slist_append(todo.scroll->list, todo.message);
	pango_layout_set_markup(todo.message, str, strlen(str));
}

static void free_items(void)
{
	int i;
	struct display_item *di;

	if (todo.message) {
		g_object_unref(todo.message);
		todo.message = NULL;
	}

	for (i=0; (di = g_slist_nth_data(items, i)); i++) {
		g_free(di->ti);
		g_object_unref(di->pl);
		g_free(di);
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

	todo.message = NULL;

	/* todo db full path */
	db_fname = g_strdup_printf("%s/%s", g_get_home_dir(), TODO_DB);

	todo.toplevel = gtk_hbox_new(FALSE, 0);

	/* todo icon */
	vboxlogo = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(todo.toplevel), vboxlogo, FALSE, FALSE, 5);

	logo = gtk_image_new_from_file(IMAGEPATH(tasks.png));
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

	if (todo_db_start())
		return;
	else
		db_opened = TRUE;

	free_items();

	for (iter = todo_db_get_items_list(); iter; iter = g_slist_next(iter)) {
		if (((struct todo_item *)iter->data)->state == COMPLETED)
			continue;

		di = g_malloc(sizeof(struct display_item));
		items = g_slist_append(items, di);
		di->ti = iter->data;

		di->pl = gtk_widget_create_pango_layout(todo.scroll->draw, NULL);
		pango_layout_set_wrap(di->pl, PANGO_WRAP_WORD);
		todo.scroll->list = g_slist_append(todo.scroll->list, di->pl);
		di->overdue = FALSE;
		refresh_todo_markup(di);
	}

	if (g_slist_length(items) == 0)
		make_no_items_label();
}

static void refresh_todo_markup(struct display_item *di)
{
	char *color, *red = "red", *black = "black", *green = "green";
	time_t t = time(NULL);

	if (di->ti->time != 0 && di->ti->time < mktime(localtime(&t))) {
		di->overdue = TRUE;
		color = red;
	} else if (di->ti->state == IN_PROGRESS)
		color = green;
	else
		color = black;

	markup_printf(di->pl, "<span foreground=\"%s\">%s</span>", color, di->ti->summary);
}

gboolean todo_update(gpointer data)
{
	struct stat db;
	static time_t prev_modified = 0;
	time_t current_time;
	GSList *iter;

	if (stat(db_fname, &db) == -1) {
		if (g_slist_length(items) != 0)
			free_items();
		make_no_items_label();
		return TRUE;
	}

	if (db.st_mtime > prev_modified) {
		prev_modified = db.st_mtime;
		todo_db_update();
	}

	current_time = time(NULL);
	current_time = mktime(localtime(&current_time));

	for (iter = items; iter; iter = g_slist_next(iter)) {
		struct display_item *di = iter->data;
		
		if (!di->overdue && di->ti->time != 0 && di->ti->time < current_time)
			refresh_todo_markup(di);
	}

	refresh_todo_widget();

	return TRUE;
}
