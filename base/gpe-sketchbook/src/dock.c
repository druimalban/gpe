/* GPE Dock -  a dock to handle toolbars orientation changes
 * Copyright (C) 2002 Luc Pionchon <luc.pionchon@welho.com>
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
 */

/*
  TODO:
   - make it as a GtkContainer::GpeDock
   - allow multiple toolbars (or menus)
   - allow several positions (especialy left/right)
*/
#include <gtk/gtk.h>
#include <stdlib.h> //malloc() free()

#include "dock.h"

/** creates a new empty dock */
GpeDock * gpe_dock_new(){
  GpeDock * dock;
  dock = (GpeDock *) malloc(sizeof(GpeDock));
  dock->app_content = NULL;
  dock->toolbar     = NULL;
  dock->_table      = (GtkTable *)gtk_table_new (3, 3, FALSE);
  return dock;
}

/** destroys a dock (and its childs) */
void gpe_dock_destroy(GpeDock * dock){
  if(dock == NULL) return;
  if(dock->app_content != NULL) gtk_widget_destroy(dock->app_content);
  if(dock->toolbar     != NULL) gtk_widget_destroy((GtkWidget *)dock->toolbar);
  if(dock->_table      != NULL) gtk_widget_destroy((GtkWidget *)dock->_table);
  free(dock);
}

/** returns the dock main container,
    this is just a workaround until GpeDock is a GtkWidget
 */
GtkWidget * gpe_dock(GpeDock * dock){
  if(dock == NULL) return NULL;
  else return (GtkWidget *) dock->_table;
}

/** adds the application main "area" to the dock */
void gpe_dock_add_app_content(GpeDock * dock,
                              GtkWidget * content){
  if(dock == NULL) return;
  if(dock->app_content != NULL) return;
  if(content == NULL) return;
  dock->app_content = content;
  gtk_table_attach_defaults (dock->_table, content, 1, 2, 1, 2);
}

/** adds the toolbar to the dock */
void gpe_dock_add_toolbar(GpeDock * dock,
                          GtkToolbar * toolbar,
                          GtkOrientation orientation){
  if(dock == NULL) return;
  if(dock->toolbar != NULL) return; //already one
  if(toolbar == NULL) return;

  gtk_toolbar_set_orientation (toolbar, orientation);

  if(orientation == GTK_ORIENTATION_VERTICAL){
    gtk_table_attach (dock->_table, (GtkWidget *)toolbar, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  }
  else{//GTK_ORIENTATION_HORIZONTAL
    gtk_table_attach (dock->_table, (GtkWidget *)toolbar, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  }

  dock->toolbar = toolbar;
  dock->toolbar_orientation = orientation;
}


void gtk_table_remove (GtkContainer *container,
                       GtkWidget    *widget){
  // Function borrowed from gtk-1.2/gtktable.c
  // GTK - The GIMP Toolkit
  // Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
  //
  // Changed "unparent()" to "parent = NULL"
  //
  GtkTable *table;
  GtkTableChild *child;
  GList *children;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_TABLE (container));
  g_return_if_fail (widget != NULL);

  table = GTK_TABLE (container);
  children = table->children;

  while (children)
    {
      child = children->data;
      children = children->next;

      if (child->widget == widget)
        {
          gboolean was_visible = GTK_WIDGET_VISIBLE (widget);

          //gtk_widget_unparent (widget);
          widget->parent = NULL;

          table->children = g_list_remove (table->children, child);
          g_free (child);

          if (was_visible && GTK_WIDGET_VISIBLE (container))
            gtk_widget_queue_resize (GTK_WIDGET (container));
          break;
        }
    }
}


/** changes the orientation of the toolbar */
void gpe_dock_change_toolbar_orientation(GpeDock * dock,
                                         GtkOrientation orientation){
  if(dock == NULL) return;
  if(dock->toolbar == NULL) return;
  if(dock->toolbar_orientation == orientation) return;
  //**/g_printerr("Changing orientation %d->%d\n", dock->toolbar_orientation, orientation);
  
  gtk_toolbar_set_orientation (dock->toolbar, orientation);
  dock->toolbar_orientation = orientation;

  //**/g_printerr("tb %s\n", GTK_IS_WIDGET(dock->toolbar)?"OK":"NOT ok");
  //gtk_widget_unparent((GtkWidget *)dock->toolbar);
  //gtk_widget_destroy ((GtkWidget *)dock->toolbar);
  //gtk_container_remove((GtkContainer *)dock->_table, (GtkWidget *)dock->toolbar);
  gtk_table_remove ((GtkContainer *)dock->_table, (GtkWidget *)dock->toolbar);
  //((GtkWidget *)(dock->toolbar))->parent = NULL;
  //**/g_printerr("tb %s\n", GTK_IS_WIDGET(dock->toolbar)?"OK":"NOT ok");
  //**/g_printerr("tb %s\n", (dock->toolbar == NULL)?"NULL":"NOT null");

  if(orientation == GTK_ORIENTATION_VERTICAL){
    gtk_table_attach (dock->_table, (GtkWidget *)dock->toolbar, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  }
  else{//GTK_ORIENTATION_HORIZONTAL
    gtk_table_attach (dock->_table, (GtkWidget *)dock->toolbar, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_FILL), (GtkAttachOptions) (GTK_FILL), 0, 0);
  }
  //gtk_widget_queue_resize ((GtkWidget *)dock->_table);
  //gtk_widget_show_now((GtkWidget *)dock->app_content);
  //gtk_widget_show_now((GtkWidget *)dock->toolbar);
  //gtk_widget_show_now((GtkWidget *)dock->_table);
  gtk_signal_emit_by_name((GtkObject *)dock->_table, "check-resize");
  //gtk_widget_queue_resize (GTK_WIDGET (container));

}
