/*
 * Copyright (C) 2002, 2003 Robert Mibus <mibus@handhelds.org>,
 *               Philip Blundell <philb@gnu.org>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <libintl.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "question.h"
#include "spacing.h"
#include "picturebutton.h"
#include "pixmaps.h"
#include "link-warning.h"

#define _(x) dgettext(PACKAGE, x)

static void
on_qn_button_clicked (GtkButton *button, gpointer user_data)
{
  gint *ret = gtk_object_get_data (GTK_OBJECT (user_data), "return");
  *ret = (gint)gtk_object_get_data (GTK_OBJECT (button), "value");

  gtk_widget_destroy (user_data);

  gtk_main_quit();
}

static void
add_button (char *text, char *icon, GtkWidget *dialog, GtkWidget *box, int spacing, int value)
{
  GtkWidget *btn;

  if (text[0] == '!')
    btn = gpe_button_new_from_stock (text + 1, GPE_BUTTON_TYPE_BOTH);
  else
    btn = gpe_picture_button (dialog->style, text, icon);

  g_object_set_data (G_OBJECT (btn), "value", (gpointer) value);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (on_qn_button_clicked),
		    dialog);
  gtk_box_set_spacing (GTK_BOX (box), spacing);
  gtk_box_pack_start (GTK_BOX (box), btn, TRUE, FALSE, 0);
}

gint
gpe_question_ask (char *qn, char *title, char *iconname, ...)
{
  GtkWidget *window, *hbox, *label;
  GdkPixbuf *p;
  gint button_pressed = -1;
  int i = 0;
  va_list ap;

  /* alerts seem to use some non-standard values
   *  (http://developer.gnome.org/projects/gup/hig/1.0/windows.html#alert-spacing)
   */
  guint gpe_boxspacing = 12/GPE_GNOME_SCALING;
  guint gpe_border     = gpe_boxspacing/2;

  /* -------------------------------------------------- */
  /* top level window
   */
  window = gtk_dialog_new ();
  gtk_dialog_set_has_separator (GTK_DIALOG (window), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (window), gpe_border);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (window)->vbox), gpe_boxspacing);
  /* See the following mail about UrgencyHint:
   * http://mail.gnome.org/archives/usability/2002-January/msg00088.html
   */
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_main_quit),
		    NULL);
  gtk_object_set_data (GTK_OBJECT (window), "return", &button_pressed);

  /* -------------------------------------------------- */
  /* hbox: left column: icon
   *       right column: label
   */
  hbox = gtk_hbox_new (FALSE, gpe_boxspacing);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), gpe_border);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (window)->vbox), hbox);

  /* icon / label */
  if (iconname[0] == '!')
    {
      GtkWidget *icon = gtk_image_new_from_stock (iconname + 1, GTK_ICON_SIZE_DIALOG);
      gtk_misc_set_alignment (GTK_MISC (icon), 0, 0);
      gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
    }
  else
    {
      p = gpe_try_find_icon (iconname, NULL);
      if (p != NULL)
	{
	  GtkWidget *icon = gtk_image_new_from_pixbuf (p);
	  gtk_misc_set_alignment (GTK_MISC (icon), 0, 0);
	  gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, TRUE, 0);
	}
    }

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), qn);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);

  /* -------------------------------------------------- */
  /* put a user-defined number of buttons into the button box
   */
  va_start (ap, iconname);
  for (;;)
    {
      char *btn_lbl, *btn_icon;

      btn_lbl = va_arg (ap, char *);
      if (btn_lbl == NULL)
	break;
      
      btn_icon = va_arg (ap, char *);
      
      add_button (btn_lbl, btn_icon, window, GTK_DIALOG (window)->action_area, gpe_boxspacing/2, i++);
    }
  va_end (ap);

  /* -------------------------------------------------- */
  if (title && title[0])
    {
      gtk_window_set_title (GTK_WINDOW (window), title);
    }
  else
    {
      gtk_window_set_title (GTK_WINDOW (window), "");
      gtk_widget_realize (window);
      gdk_window_set_decorations (window->window, GDK_DECOR_BORDER);
      /* Maybe also set
	 gdk_window_set_functions (); */
  }

  gtk_widget_show_all (window);

  gtk_main ();

  return button_pressed;
}

gint
gpe_question_ask_yn (char *qn)
{
  return gpe_question_ask (qn, _("Question"), "question",
			   "!gtk-no", NULL, "!gtk-yes", NULL, NULL);
}
link_warning(gpe_question_ask_yn, 
	     "warning: gpe_question_ask_yn is obsolescent: use gpe_question_ask directly.");
