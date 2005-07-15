/* GPE Life
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include "l-view.h"
#include "l-model.h"
#include "gui.h"
#include "gpe-life-i18n.h"

struct _LMainWindow
{
  GtkWindow *window;
  GtkToolButton *play_button;
  GtkToolButton *zoom_out_button;
  GtkStatusbar *status_bar;
  GtkWidget *view;
};
typedef struct _LMainWindow LMainWindow;

static gboolean running = FALSE;

gint
delete_event (GtkWidget * widget, GdkEvent event, gpointer data)
{
  /* just close the window */
  return FALSE;
}

void
gpe_life_exit (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

void
generation_changed (guint generation, gpointer data)
{
  GtkStatusbar *statusbar = ((LMainWindow *) data)->status_bar;
  guint id;
  gchar *message;
  
  message = g_strdup_printf ("Generation: %d", generation);
  id = gtk_statusbar_get_context_id (statusbar, "generation change");
  gtk_statusbar_pop (statusbar, id);
  gtk_statusbar_push (statusbar, id, message);

  g_free (message);
}

void
new_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *view = ((LMainWindow *) data)->view;
  GtkToolButton *play_button = ((LMainWindow *) data)->play_button;
  guint interval = l_model_get_update_interval (l_view_get_model (view));

  l_view_set_model (view, l_model_new(interval));
  running = FALSE;
  gtk_tool_button_set_stock_id (play_button, GTK_STOCK_MEDIA_PLAY);

  l_model_add_generation_notify (l_view_get_model (view), generation_changed, (LMainWindow *) data);
  generation_changed (0, (LMainWindow *) data);
}

void
run_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *view = ((LMainWindow *) data)->view;

  running = !running;
  l_model_run (l_view_get_model(view), running);

  gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (widget), running ? GTK_STOCK_MEDIA_PAUSE : GTK_STOCK_MEDIA_PLAY);
}

void
zoom_out_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *view = ((LMainWindow *) data)->view;

  if (! l_view_dec_cell_size (view))
    gtk_widget_set_sensitive (widget, FALSE);
}

void
zoom_in_clicked (GtkWidget *widget, gpointer data)
{
  GtkWidget *view = ((LMainWindow *) data)->view;
  GtkToolButton *zoom_out = ((LMainWindow *) data)->zoom_out_button;

  l_view_inc_cell_size (view);
  gtk_widget_set_sensitive (GTK_WIDGET (zoom_out), TRUE);
}

int
main(int argc, char **argv )
{
  GladeXML *ui;
  LMainWindow main_window;
  GtkWidget *drawing_area;
  GtkWidget *scrolled_window;
  LModel *model;
  /*GtkAdjustment *adjustment;
  */

  bindtextdomain (GETTEXT_PACKAGE, GPEMILEAGELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  ui = gui_glade_xml_new ("main_window");
  main_window.window = GTK_WINDOW (gui_glade_xml_get_widget (ui, "main_window"));
  main_window.play_button = GTK_TOOL_BUTTON (gui_glade_xml_get_widget (ui, "play_button"));
  main_window.zoom_out_button = GTK_TOOL_BUTTON (gui_glade_xml_get_widget (ui, "zoom_out_button"));
  main_window.status_bar = GTK_STATUSBAR (gui_glade_xml_get_widget (ui, "statusbar"));
  scrolled_window = GTK_WIDGET (gui_glade_xml_get_widget (ui, "scrolledwindow1"));

  /* standard window signals */
  g_signal_connect (G_OBJECT (main_window.window), "delete-event",
                    G_CALLBACK (delete_event), NULL);

  g_signal_connect (G_OBJECT (main_window.window), "destroy",
                    G_CALLBACK (gpe_life_exit), NULL);

  /* create the Life view */
  drawing_area = l_view_new ();
  main_window.view = drawing_area;

  gtk_widget_set_size_request (GTK_WIDGET (drawing_area), 8, 8);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), GTK_WIDGET (drawing_area));

  /* assign model */
  model = l_model_new(200);
  l_view_set_model (drawing_area, model);

  l_model_add_generation_notify (model, generation_changed, &main_window);
  generation_changed (0, &main_window);

  glade_xml_signal_connect_data (ui,
                                 "exit_button_clicked",
                                 G_CALLBACK (gpe_life_exit),
                                 &main_window);
  
  glade_xml_signal_connect_data (ui,
                                 "play_button_clicked",
                                 G_CALLBACK (run_clicked),
                                 &main_window);
  
  glade_xml_signal_connect_data (ui,
                                 "new_button_clicked",
                                 G_CALLBACK (new_clicked),
                                 &main_window);
  
  glade_xml_signal_connect_data (ui,
                                 "zoom_in_button_clicked",
                                 G_CALLBACK (zoom_in_clicked),
                                 &main_window);
  
  glade_xml_signal_connect_data (ui,
                                 "zoom_out_button_clicked",
                                 G_CALLBACK (zoom_out_clicked),
                                 &main_window);
  
  /* FIXME: Why doesn't this have any effect?
  adjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
  gtk_adjustment_set_value (adjustment, 0.5);
  gtk_adjustment_changed (adjustment);
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (scrolled_window), adjustment);
  fprintf (stdout, "adj: lower=%f, upper=%f, new=%f\n", adjustment->lower, adjustment->upper, gtk_adjustment_get_value (adjustment));
  gtk_adjustment_changed (adjustment);

  adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
  gtk_adjustment_set_value (adjustment, 0.5);
  //gtk_adjustment_set_value (adjustment, (((adjustment->upper - adjustment->page_size) - adjustment->lower)/2) +  adjustment->lower);
  adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window));
  fprintf (stdout, "adj: lower=%f, upper=%f, page-size=%f, new=%f\n", adjustment->lower, adjustment->upper, adjustment->page_size, gtk_adjustment_get_value (adjustment));
  gtk_adjustment_changed (adjustment);
  gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW (scrolled_window), adjustment);
  gtk_adjustment_changed (adjustment);
  */

  gtk_widget_show_all (GTK_WIDGET (main_window.window));

  gtk_main ();

  return 0;
}

