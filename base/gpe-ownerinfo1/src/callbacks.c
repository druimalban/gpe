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
on_smallphotobutton_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *notebook;

  notebook = lookup_widget (GTK_WIDGET (button), "notebook");
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 1);
}

void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  GtkWidget *notebook;

  notebook = lookup_widget (GTK_WIDGET (button), "notebook");
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);
  gtk_widget_queue_draw (GTK_WIDGET (notebook));
}

void
on_bigphotobutton_size_allocate        (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *bigphotobutton;
  GtkWidget *bigphoto;

  /*
  g_message("bigphotobutton allocation.width: %d", allocation->width);
  g_message("bigphotobutton allocation.height: %d", allocation->height);
  */

  bigphotobutton = lookup_widget (widget, "bigphotobutton");
  window = lookup_widget (widget, "GPE_Ownerinfo");
  bigphoto = create_pixmap (window, "ownerphoto");

  /*
   * gtk_container_children() brought to me by PaxAnima. Thanks.
   * It looks like in GTK2, this is gtk_container_get_children().
   */
  if (gtk_container_children (GTK_CONTAINER (bigphotobutton)) == NULL) {
    gtk_widget_show (bigphoto);
    gtk_container_add (GTK_CONTAINER (bigphotobutton), bigphoto);
  }
}

void
on_smallphotobutton_size_allocate      (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data)
{
  GtkWidget *window;
  GtkWidget *smallphotobutton;
  GtkWidget *smallphoto;

  /*
  g_message("smallphotobutton allocation.width: %d", allocation->width);
  g_message("smallphotobutton allocation.height: %d", allocation->height);
  */

  smallphotobutton = lookup_widget (widget, "smallphotobutton");
  window = lookup_widget (widget, "GPE_Ownerinfo");
  smallphoto = create_pixmap (window, "ownerphoto");

  /*
   * gtk_container_children() brought to me by PaxAnima. Thanks.
   * It looks like in GTK2, this is gtk_container_get_children().
   */
  if (gtk_container_children (GTK_CONTAINER (smallphotobutton)) == NULL) {
    gtk_widget_show (smallphoto);
    /* display photo aligned on top */
    gtk_misc_set_alignment (GTK_MISC (smallphoto), 0.5, 0);
    gtk_container_add (GTK_CONTAINER (smallphotobutton), smallphoto);
  }
}
