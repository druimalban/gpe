/*
 *  Copyright (C) 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <locale.h>
#include <libintl.h>
#define _(x) gettext(x)

#include <gtk/gtk.h>
#include <gpe/spacing.h>
#include "feededit.h"

#define IPKG_FEEDCONFIG "/tmp/feeds.conf"

extern GtkWidget *fMain;
static GtkListStore *feedstore;

static void
read_feeds_from_file(const gchar *filename)
{
	FILE *fp = fopen(filename, "r");
	char buf[255];
	char name[64], url[255], stype[10];
	int type = FT_PROTECTED;
	GtkTreeIter iter;
	
	if (fp != NULL)
	{
		while (fgets(buf, 255, fp))
		{
			g_strstrip(buf);
			
			if ((buf[0] != '#') 
				&& (3 == sscanf(buf, "%10s %64s %255s", stype, name, url)))
			{
				if (!strcmp(stype, "src"))
					type = FT_UNCOMPRESED;
				else if (!strcmp(stype, "src/gz"))
					type = FT_COMPRESSED;
				gtk_list_store_append(feedstore, &iter);
				gtk_list_store_set(feedstore, &iter,
				                   FEED_NAME, name,
				                   FEED_URL, url,
				                   FEED_TYPE, type, -1);
			}
		}	
	}
}

void
on_add_feed(GtkWidget *button, gpointer data)
{
	
}

void
on_remove_feed(GtkWidget *button, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeSelection *sel;
	GtkWidget *treeview = data;
	GtkWidget *dialog;
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if (gtk_tree_selection_get_selected(sel, 
		                               (GtkTreeModel**)(&feedstore), &iter)) 
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (fMain),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Delete selected feed?"));
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) 
		{
			gtk_list_store_remove(feedstore , &iter);
		}
		gtk_widget_destroy(dialog);
	}
}

void
on_edit_feed(GtkWidget *button, gpointer data)
{
	
}

GtkWidget *
create_feed_edit(void)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, gpe_get_boxspacing());
	GtkWidget *toolbar = gtk_toolbar_new();
	GtkWidget *label = gtk_label_new(NULL);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *treeview, *vp, *sw;
	gchar *ts;
	
	feedstore = gtk_list_store_new(FEED_FIELDNUM, 
	                               G_TYPE_INT,
				                   G_TYPE_STRING,
								   G_TYPE_STRING);
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (feedstore));
	
	ts = g_strdup_printf("<b>%s</b>", _("Package Feeds"));
	gtk_label_set_markup(GTK_LABEL(label), ts);
	g_free(ts);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar), GTK_ORIENTATION_HORIZONTAL);
	
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), 
	                         GTK_STOCK_ADD, 
	                         _("Add a new package feed."),
	                         NULL, G_CALLBACK(on_add_feed), treeview, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), 
	                         GTK_STOCK_REMOVE, 
	                         _("Remove selected package feed."),
	                         NULL, G_CALLBACK(on_remove_feed), treeview, -1);
	gtk_toolbar_insert_stock(GTK_TOOLBAR(toolbar), 
	                         GTK_STOCK_PREFERENCES, 
	                         _("Edit selected package feed."),
	                         NULL, G_CALLBACK(on_edit_feed), treeview, -1);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
	
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(treeview),TRUE);
	sw = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), 
	                               GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	vp = gtk_viewport_new(NULL, NULL);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(vp), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(vp), treeview);
	gtk_container_add(GTK_CONTAINER(sw), vp);
	gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
							   renderer,
							   "text",
							   FEED_NAME,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Location"),
							   renderer,
							   "text",
							   FEED_URL,
							   NULL);
	gtk_tree_view_column_set_resizable(GTK_TREE_VIEW_COLUMN(column), TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
	read_feeds_from_file(IPKG_FEEDCONFIG);
	
	return vbox;
}
