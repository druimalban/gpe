
#include <gtk/gtk.h>
#include "gtk_graphic_funcs.h"

/* -----------------------------------------------------
 * create_widget_from_xpm
 * Convert an xpm string into a pixmap widget
 * ----------------------------------------------------- */

GtkWidget *widget_new_from_xpm (GtkWidget *window, gchar **xpm_data)
{
  GdkPixmap *pixmap_data;
  GdkBitmap *mask;
  GtkStyle  *style;
  GtkWidget *pixmap_widget;

  style = gtk_widget_get_style (window);

  /* Convert the string in xpm_data to a gdk_pixmap */
  pixmap_data = gdk_pixmap_create_from_xpm_d (window->window,
					      &mask,
					      &style->bg[GTK_STATE_NORMAL],
					      (gchar **) xpm_data);

  /* Convert the pixmap to the pixmap widget to be be returned */
  pixmap_widget = gtk_pixmap_new (pixmap_data, mask);

  return (pixmap_widget);
}
