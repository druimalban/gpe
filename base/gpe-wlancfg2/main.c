/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <gtk/gtk.h>
#include <libintl.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include "interface.h"
#include "support.h"
#include "helptext.h"
#include "callbacks.h"

GtkWidget	*GPE_WLANCFG;

static struct gpe_icon icons[] = {
	{"new"},
	{"copy"},
	{"edit"},
	{"delete"},
	{"exit"},
	{"preferences"},
	{"icon", PREFIX "/share/pixmaps/gpe-wlancfg.png"},
	{NULL, NULL}
};


int main (int argc, char *argv[])
{
	
	GtkWidget       *vbSchemes;
	GtkWidget	*vbSimple;
	GtkWidget       *toolbar_main;
	GtkWidget       *toolbar_simple;
	GtkWidget	*pw;
	
	GtkListStore	*liststore;
	GtkTreeView	*treeview;
	GtkTreeViewColumn *tcolumn;
	GtkCellRenderer *renderer;
 	gchar *ts;
	
	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);	
	
	if (gpe_application_init (&argc, &argv) == FALSE) 
		exit (1);
	
	if (gpe_load_icons (icons) == FALSE) 
		exit (1);	
		
	add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");

	GPE_WLANCFG = create_GPE_WLANCFG ();

	gtk_widget_realize (GPE_WLANCFG);
  
	gpe_set_window_icon(GPE_WLANCFG, "icon");

	vbSchemes = lookup_widget (GPE_WLANCFG, "vbSchemes");

	toolbar_main = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar_main), GTK_ORIENTATION_HORIZONTAL);
	
	gtk_box_pack_start (GTK_BOX (vbSchemes), toolbar_main, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (vbSchemes), toolbar_main, 0);
	gtk_widget_show (toolbar_main);
	
	pw = gtk_image_new_from_pixbuf(gpe_find_icon ("new"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar_main), _("New profile"),
		_("New profile"), _("New profile"), pw,
		 (GtkSignalFunc) on_btnNew_clicked, NULL);
	
	pw = gtk_image_new_from_pixbuf(gpe_find_icon ("edit"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar_main), _("Edit profile"),
		_("Edit profile"), _("Edit profile"), pw,
		 (GtkSignalFunc) on_btnEdit_clicked, NULL);
	
	pw =  gtk_image_new_from_pixbuf(gpe_find_icon ("delete"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar_main), _("Delete profile"),
		_("Delete profile"), _("Delete profile"), pw,
		 (GtkSignalFunc) on_btnDelete_clicked, NULL);
	
	/* to switch to basic settings */
	pw =  gtk_image_new_from_pixbuf(gpe_find_icon ("preferences"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar_main), _("Basic settings"),
		_("Basic settings"), _("Basic settings"), pw,
		 (GtkSignalFunc) on_btnExpert_clicked, (void*)1);
	
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar_main));
	
	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar_main), GTK_STOCK_QUIT,_("Exit"),
		_("Exit"), (GtkSignalFunc) on_GPE_WLANCFG_de_event, (gpointer)3, -1);
	
	vbSimple = lookup_widget (GPE_WLANCFG, "vbSimple");

	toolbar_simple = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar_simple), GTK_ORIENTATION_HORIZONTAL);
	
	gtk_box_pack_start (GTK_BOX (vbSimple), toolbar_simple, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (vbSimple), toolbar_simple, 0);
	gtk_widget_show (toolbar_simple);
	
	/* to switch to expert settings */
	pw =  gtk_image_new_from_pixbuf(gpe_find_icon ("preferences"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar_simple), _("Expert settings"),
		_("Expert settings"), _("Expert settings"), pw,
		 (GtkSignalFunc) on_btnExpert_clicked, (void*)0);

	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar_simple), GTK_STOCK_HELP,_("Help"),
		_("Fulltext help"), (GtkSignalFunc) on_btnHelp_clicked, NULL, -1);

	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar_simple));

	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar_simple), GTK_STOCK_APPLY,_("Save settings"),
		NULL, (GtkSignalFunc) on_GPE_WLANCFG_de_event, (gpointer)FALSE, -1);

	gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar_simple), GTK_STOCK_QUIT,_("Exit"),
		_("Exit"), (GtkSignalFunc) on_GPE_WLANCFG_de_event, (gpointer)TRUE, -1);

	
	treeview=GTK_TREE_VIEW(lookup_widget(GPE_WLANCFG, "tvSchemeList"));
	
	liststore=gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
	
	gtk_tree_view_set_model(treeview, GTK_TREE_MODEL(liststore));
	gtk_tree_view_set_rules_hint (treeview, TRUE);

	renderer = gtk_cell_renderer_text_new ();
	tcolumn = gtk_tree_view_column_new_with_attributes (_("Scheme"),
						     renderer,
						     "text",
						     0,
						     NULL);
						     
	gtk_tree_view_append_column (treeview, tcolumn);
	
	renderer = gtk_cell_renderer_text_new ();
	tcolumn = gtk_tree_view_column_new_with_attributes (_("Socket"),
						     renderer,
						     "text",
						     1,
						     NULL);
						     
	gtk_tree_view_append_column (treeview, tcolumn);
	
	renderer = gtk_cell_renderer_text_new ();
	tcolumn = gtk_tree_view_column_new_with_attributes (_("Instance"),
						     renderer,
						     "text",
						     2,
						     NULL);
						     
	gtk_tree_view_append_column (treeview, tcolumn);
	
	renderer = gtk_cell_renderer_text_new ();
	tcolumn = gtk_tree_view_column_new_with_attributes (_("MAC"),
						     renderer,
						     "text",
						     3,
						     NULL);
						     
	gtk_tree_view_append_column (treeview, tcolumn);
	
	/* set some labels bold */
	pw = lookup_widget (GPE_WLANCFG, "lblHeader_general_simple");
	ts = g_strdup_printf("<b>%s</b>",gtk_label_get_text(GTK_LABEL(pw)));
	gtk_label_set_markup(GTK_LABEL(pw),ts);
	g_free(ts);

	pw = lookup_widget (GPE_WLANCFG, "lblHeader_encryption_simple");
	ts = g_strdup_printf("<b>%s</b>",gtk_label_get_text(GTK_LABEL(pw)));
	gtk_label_set_markup(GTK_LABEL(pw),ts);
	g_free(ts);

	init_help();

	gtk_widget_show (GPE_WLANCFG);

	gtk_main ();
	return 0;
}
