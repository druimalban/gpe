#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"


void
on_photobutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;

  GtkWidget *name;

  widget = lookup_widget (GTK_WIDGET (button), "table1");
  gtk_widget_hide (GTK_WIDGET (widget));

  name = gtk_label_new ("Foo Bar");
  gtk_widget_show (GTK_WIDGET (name));

  widget = lookup_widget (GTK_WIDGET (button), "frame1");
  gtk_container_add (GTK_CONTAINER (widget), name);
  gtk_widget_show (GTK_WIDGET (widget));

  // bigphotobutton = gpe_render_icon (widget, gpe_find_icon ("tux-48"));
  // gtk_widget_show (bigphotobutton);

}

void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *widget;

  widget = lookup_widget (GTK_WIDGET (button), "bigphotobutton");
  gtk_widget_hide (GTK_WIDGET (widget));

  widget = lookup_widget (GTK_WIDGET (button), "table1");
  gtk_widget_show (GTK_WIDGET (widget));

}

