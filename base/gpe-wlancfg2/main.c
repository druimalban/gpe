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

#include <unistd.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libintl.h>


#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "interface.h"
#include "support.h"

GtkWidget	*GPE_WLANCFG;
static struct gpe_icon icon = {"icon", PREFIX "/share/pixmaps/gpe-wlancfg.png"};


int main (int argc, char *argv[])
{
	
	GtkListStore	*liststore;
	GtkTreeView	*treeview;
	GtkTreeViewColumn *tcolumn;
	GtkCellRenderer *renderer;
	GdkPixmap 	*pmap;
	GdkBitmap 	*bmap;

	setlocale (LC_ALL, "");

	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);
	
//	gtk_set_locale ();	
//	gtk_init (&argc, &argv);
	
	if (gpe_application_init (&argc, &argv) == FALSE) exit (1);
	
	add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");

	GPE_WLANCFG = create_GPE_WLANCFG ();
  
	if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
		gdk_window_set_icon (GPE_WLANCFG->window, NULL, pmap, bmap);

	
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

	gtk_widget_show (GPE_WLANCFG);

	gtk_main ();
	return 0;
}

