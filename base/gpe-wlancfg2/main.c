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
//#include <libintl.h>

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
	{"icon", PREFIX "/share/pixmaps/gpe-wlancfg.png"},
	{NULL, NULL}
};


int main (int argc, char *argv[])
{
	
	GtkWidget       *vbSchemes;
	GtkWidget       *toolbar;
	GtkWidget	*pw;
	
	GtkListStore	*liststore;
	GtkTreeView	*treeview;
	GtkTreeViewColumn *tcolumn;
	GtkCellRenderer *renderer;
 
	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);	
	
	if (gpe_application_init (&argc, &argv) == FALSE) 
		exit (1);
	
	if (gpe_load_icons (icons) == FALSE) 
		exit (1);	
		
	add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");

	GPE_WLANCFG = create_GPE_WLANCFG ();

	gtk_widget_realize (GPE_WLANCFG);
  
	/* now depricated?
	  
	if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
		gdk_window_set_icon (GPE_WLANCFG->window, NULL, pmap, bmap);
	
	*/
	gpe_set_window_icon(GPE_WLANCFG, "icon");

	vbSchemes = lookup_widget (GPE_WLANCFG, "vbSchemes");

	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
	
	gtk_box_pack_start (GTK_BOX (vbSchemes), toolbar, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (vbSchemes), toolbar, 0);
	gtk_widget_show (toolbar);
	
	pw = gtk_image_new_from_pixbuf(gpe_find_icon ("new"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New profile"),
		_("New profile"), _("New profile"), pw,
		 (GtkSignalFunc) on_btnNew_clicked, NULL);
	
	pw = gtk_image_new_from_pixbuf(gpe_find_icon ("edit"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Edit profile"),
		_("Edit profile"), _("Edit profile"), pw,
		 (GtkSignalFunc) on_btnEdit_clicked, NULL);
	
	pw =  gtk_image_new_from_pixbuf(gpe_find_icon ("delete"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete profile"),
		_("Delete profile"), _("Delete profile"), pw,
		 (GtkSignalFunc) on_btnDelete_clicked, NULL);
	gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
	
	pw =  gtk_image_new_from_pixbuf(gpe_find_icon ("exit"));
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"),
		_("Exit"), _("Exit"), pw, (GtkSignalFunc) on_GPE_WLANCFG_de_event, NULL);
	
	
	
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

	init_help();

	gtk_widget_show (GPE_WLANCFG);

	gtk_main ();
	return 0;
}
