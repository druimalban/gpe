#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "brightness.h"
#include "calibrate.h"
#include "rotation.h"

void
on_brightness_hscale_draw              (GtkWidget       *widget,
                                        GdkRectangle    *area,
                                        gpointer         user_data)
{
  GtkAdjustment *adjustment;
  adjustment = gtk_range_get_adjustment (GTK_RANGE(widget));
  set_brightness ((int) (adjustment->value * 2.55));
}


void
on_rotation_entry_changed              (GtkEditable     *editable,
                                        gpointer         user_data)
{
  gchar *text;
  text = gtk_editable_get_chars (editable, 0, -1);
  if (strcmp (text, _("Portrait")) == 0)
    {
      set_rotation ("normal");
    }
  else if (strcmp (text, _("Inverted")) == 0)
    {
      set_rotation ("inverted");
    }
  else if (strcmp (text, _("Landscape (left)")) == 0)
    {
      set_rotation ("left");
    }
  else if (strcmp (text, _("Landscape (right)")) == 0)
    {
      set_rotation ("right");
    }
  g_free (text);
}


void
on_calibrate_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  calibrate ();
}


void
on_closebutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_exit (0);
}

gboolean
on_mainwindow_destroy_event            (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  return FALSE;
}


void
on_mainwindow_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
  gtk_exit (0);
}

