/* gtk_style_funcs.c
 * Some functions to make the creation of a widget styles simple
 */

#include <gtk/gtk.h>
#include "gtk_style_funcs.h"

/* -------------------------------------------
 * Here are all the subroutine implementations
 * ------------------------------------------- */

GtkStyle *create_new_style (GdkColor bg_color)
{
    GtkStyle *default_style;
    GtkStyle *style;
    int i;

    /* --- Get the default style --- */
    default_style = gtk_widget_get_default_style ();

    /* --- Make a copy of it. --- */
    style = gtk_style_copy (default_style);

    /* --- Set the colors for each state --- */
    for (i = 0; i < 5; i++)
      {
        /* --- Set the colors for the style --- */
        style->bg[i] = bg_color;
      }

    return (style);
}


GtkStyle *get_default_style (void)
{
    GtkStyle *default_style;

    /* --- Get the default style --- */
    default_style = gtk_widget_get_default_style ();

    return (default_style);
}
