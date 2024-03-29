/*
 *  Copyright (C) 2004, 2005, 2006  Florian Boor <florian.boor@kernelconcepts.de>
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
#include <gpe/errorbox.h>
#include <sys/types.h>
#include <dirent.h>

#include "feededit.h"

#define IPKG_FEEDCONFIG "/etc/ipkg/feeds.conf"
#define IPKG_CONFIG     "/etc/ipkg.conf"
#define IPKG_CONFIGDIR  "/etc/ipkg"

extern GtkWidget *fMain;
static GtkListStore *feedstore;
static GtkToolItem *bAdd, *bRemove, *bEdit;

void
feed_edit_set_active (gboolean active)
{
	if (active)
	  {
		  gtk_widget_show (GTK_WIDGET (bAdd));
		  gtk_widget_show (GTK_WIDGET (bRemove));
		  gtk_widget_show (GTK_WIDGET (bEdit));
	  }
	else
	  {
		  gtk_widget_hide (GTK_WIDGET (bAdd));
		  gtk_widget_hide (GTK_WIDGET (bRemove));
		  gtk_widget_hide (GTK_WIDGET (bEdit));
	  }
}

gboolean 
write_feed(GtkTreeModel *model, GtkTreePath *path,
           GtkTreeIter *iter,
           gpointer data)
{
FILE *fp = data;
gchar *name = NULL, *url = NULL;
gint type;
gboolean *editable;

	gtk_tree_model_get (GTK_TREE_MODEL(model), iter, 
	                    FEED_NAME, &name, 
	                    FEED_URL, &url,
                        FEED_TYPE, &type, 
	                    FEED_EDITABLE, &editable, -1);
	if ((type != FT_PROTECTED) && editable)
	{
		if (type == FT_UNCOMPRESED)
			fprintf(fp, "src \t%s \t%s\n", name, url);
		else
			fprintf(fp, "src/gz \t%s \t%s\n", name, url);
		g_free(name);
		g_free(url);
	}
	return FALSE;	
}
	
static void
write_feeds_to_file(const gchar *filename)
{
	FILE *fp = fopen(filename, "w");
	
	if (fp != NULL)
	{
		gtk_tree_model_foreach(GTK_TREE_MODEL(feedstore), write_feed, fp);
		fclose(fp);
	}
	else
	{
		gchar *err = g_strdup_printf("%s %s %s", _("Could not open"),
		                             filename, _("for writing."));
		gpe_error_box(err);
		g_free(err);
	}
}

static void
edit_feed(GtkTreeIter *iter)
{
GtkWidget *dialog, *btn;
GtkWidget *table = gtk_table_new(2, 3, FALSE);
GtkWidget *cbCompressed;
GtkWidget *label;
GtkWidget *eName, *eURL;
gchar *name = NULL, *url = NULL;
gint type = FT_COMPRESSED;
	
	gtk_table_set_col_spacings(GTK_TABLE(table), gpe_get_boxspacing());
	gtk_table_set_row_spacings(GTK_TABLE(table), gpe_get_boxspacing());
	
	dialog = gtk_dialog_new_with_buttons (_("Feed Properties"),
	             GTK_WINDOW (fMain),
				 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
				 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
	btn = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, 
	                            GTK_RESPONSE_ACCEPT);
	GTK_WIDGET_SET_FLAGS(btn, GTK_CAN_DEFAULT);
	gtk_widget_grab_default(btn);
	
	label = gtk_label_new(_("Name"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	eName = gtk_entry_new_with_max_length(64);
	gtk_table_attach(GTK_TABLE(table), eName, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	label = gtk_label_new(_("Location (URL)"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	eURL = gtk_entry_new_with_max_length(255);
	gtk_table_attach(GTK_TABLE(table), eURL, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	cbCompressed = gtk_check_button_new_with_label(_("Compressed"));
	gtk_table_attach(GTK_TABLE(table), cbCompressed, 0, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
	gtk_tree_model_get(GTK_TREE_MODEL(feedstore), iter, 
	                   FEED_NAME, &name, 
					   FEED_URL, &url,
                       FEED_TYPE, &type, -1);
    if (name) 
	{
		gtk_entry_set_text(GTK_ENTRY(eName), name);
		g_free(name);
	}
    if (url) 
	{
		gtk_entry_set_text(GTK_ENTRY(eURL), url);
		g_free(url);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbCompressed), 
	                             type == FT_COMPRESSED);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table, FALSE, TRUE, 0);
	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) 
	{
		name = gtk_editable_get_chars(GTK_EDITABLE(eName), 0, -1);
		url = gtk_editable_get_chars(GTK_EDITABLE(eURL), 0, -1);
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cbCompressed)))
			type = FT_COMPRESSED;
		else
			type = FT_UNCOMPRESED;
			                             
		gtk_list_store_set(feedstore, iter, 
		                   FEED_NAME, name, 
		                   FEED_URL, url,
		                   FEED_TYPE, type, -1);
		g_free(name);
		g_free(url);
		write_feeds_to_file(IPKG_FEEDCONFIG);
	}
	gtk_widget_destroy(dialog);
}

static void
read_feeds_from_file(const gchar *filename, gboolean editable)
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
			
			if ((buf[0] != '#') && (!strncasecmp(buf, "src", 3))
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
				                   FEED_TYPE, type, 
				                   FEED_EDITABLE, editable,
				                   -1);
			}
		}
		fclose(fp);
	}
}

void
on_add_feed(GtkWidget *button, gpointer data)
{
GtkTreeIter iter;
	
	gtk_list_store_append(feedstore, &iter);
	gtk_list_store_set(feedstore, &iter, FEED_TYPE, FT_COMPRESSED, 
	                   FEED_EDITABLE, TRUE, -1);
	edit_feed(&iter);
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
					 GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("Delete selected feed?"));
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES) 
		{
			gtk_list_store_remove(feedstore , &iter);
			write_feeds_to_file(IPKG_FEEDCONFIG);
		}
		gtk_widget_destroy(dialog);
	}
}

void
on_edit_feed(GtkWidget *button, gpointer data)
{
GtkTreeIter iter;
GtkTreeSelection *sel;
GtkWidget *treeview = data;
	
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	
	if (gtk_tree_selection_get_selected(sel, 
		                               (GtkTreeModel**)(&feedstore), &iter)) 
	{
		edit_feed(&iter);
	}
}

static void
fill_feed_list(void)
{
	DIR *cfgdir;
	struct dirent *di;
		
	read_feeds_from_file(IPKG_CONFIG, FALSE);
	
	cfgdir = opendir(IPKG_CONFIGDIR);
	if (!cfgdir)
		return;
	while ( (di = readdir(cfgdir)) )
	{
		gchar *fullname;
		
		/* read all real files and filter backup files */
		if ((di->d_type == DT_REG) && (di->d_name[strlen(di->d_name)-1] != '~'))
		{
			fullname = g_strdup_printf("%s/%s", IPKG_CONFIGDIR, di->d_name);
			read_feeds_from_file(fullname, 
			                     strcmp(fullname, IPKG_FEEDCONFIG) ? FALSE : TRUE );
			g_free(fullname);
		}
	}
	closedir(cfgdir);
}

static void 
list_select_row (GtkTreeView *treeview, gpointer data)
{
	GtkTreeModel *model = data;
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	
	sel = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(sel, &model, &iter))
	{
		gboolean editable;
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter, 
        	                FEED_EDITABLE, &editable, -1);
		gtk_widget_set_sensitive(GTK_WIDGET(bEdit), editable);
		gtk_widget_set_sensitive(GTK_WIDGET(bRemove), editable);
	}
}

GtkWidget *
create_feed_edit(GtkWidget *toolbar)
{
GtkWidget *vbox = gtk_vbox_new(FALSE, gpe_get_boxspacing());
GtkWidget *label = gtk_label_new(NULL);
GtkCellRenderer *renderer;
GtkTreeViewColumn *column;
GtkWidget *treeview, *vp, *sw;
gchar *ts;
	
	feedstore = gtk_list_store_new(FEED_FIELDNUM, 
	                               G_TYPE_INT,
				                   G_TYPE_STRING,
								   G_TYPE_STRING,
	                               G_TYPE_BOOLEAN);
	
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (feedstore));
	
	ts = g_strdup_printf("<b>%s</b>", _("Package Feeds"));
	gtk_label_set_markup(GTK_LABEL(label), ts);
	g_free(ts);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
	
	bAdd = gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(bAdd), "clicked", G_CALLBACK(on_add_feed), 
	                 treeview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), bAdd, -1);
	
	bRemove = gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE);
	g_signal_connect(G_OBJECT(bRemove), "clicked", G_CALLBACK(on_remove_feed), 
	                 treeview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), bRemove, -1);

	bEdit = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
	g_signal_connect(G_OBJECT(bEdit), "clicked", G_CALLBACK(on_edit_feed), 
	                 treeview);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), bEdit, -1);

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
	
	g_signal_connect_after(G_OBJECT(treeview), "cursor-changed", 
	                       G_CALLBACK(list_select_row), (gpointer)(feedstore)); 
	
	fill_feed_list();

	gtk_widget_set_sensitive(GTK_WIDGET(bRemove), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(bEdit), FALSE);

	return vbox;
}
