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
#include "render.h"
#include "picturebutton.h"
#include "pixmaps.h"
#include "link-warning.h"

#define _(x) dgettext(PACKAGE, x)

void
on_qn_button_clicked (GtkButton *button, gpointer user_data)
{
  gint *ret = gtk_object_get_data (GTK_OBJECT (user_data), "return");
  *ret = (gint)gtk_object_get_data (GTK_OBJECT(button), "value");

  gtk_widget_destroy (user_data);

  gtk_main_quit();
}

void
add_button (char *text, char *icon, GtkWidget *dialog, GtkWidget *box, int value)
{
  GtkWidget *btn;

  if (text[0] == '!')
    btn = gpe_button_new_from_stock (text + 1, GPE_BUTTON_TYPE_BOTH);
  else
    btn = gpe_picture_button (dialog->style, text, icon);

  g_object_set_data (G_OBJECT (btn), "value", (gpointer)value);
  g_signal_connect (G_OBJECT (btn), "clicked",
		    G_CALLBACK (on_qn_button_clicked),
		    dialog);
  gtk_box_pack_start (GTK_BOX (box), btn, TRUE, FALSE, 0);
}

gint
gpe_question_ask (char *qn, char *title, char *iconname, ...)
{
  GtkWidget *window, *hbox, *label, *vbox, *buttonhbox, *sep;
  GdkPixbuf *p;
  gint button_pressed = -1;
  int i = 0;
  va_list ap;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

  vbox = gtk_vbox_new (FALSE, 0);
  buttonhbox = gtk_hbox_new (FALSE, 0);

  /* FIXME: do not hardcode the border width here, but use a global GPE constant [CM] */
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  sep = gtk_hseparator_new ();

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (gtk_main_quit),
		    NULL);

  gtk_object_set_data (GTK_OBJECT (window), "return", &button_pressed);

  /* icon / label */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (vbox), hbox);

  if (iconname[0] == '!')
    {
      GtkWidget *icon = gtk_image_new_from_stock (iconname + 1, GTK_ICON_SIZE_DIALOG);
      gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
    }
  else
    {
      p = gpe_try_find_icon (iconname, NULL);
      if (p != NULL)
	{
	  GtkWidget *icon = gtk_image_new_from_pixbuf (p);
	  gtk_box_pack_start (GTK_BOX (hbox), icon, TRUE, TRUE, 0);
	}
    }

  label = gtk_label_new (qn);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 4);

  gtk_box_pack_start (GTK_BOX (vbox), sep, FALSE, FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), buttonhbox, FALSE, FALSE, 0);

  if (title && title[0])
    {
      gtk_window_set_title (GTK_WINDOW (window), title);
      gtk_container_add (GTK_CONTAINER (window), vbox);
    }
  else
    {
      GtkWidget *frame = gtk_frame_new (NULL);
      gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
      gtk_window_set_title (GTK_WINDOW (window), "");
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_container_add (GTK_CONTAINER (window), frame);
    }

  /* buttons */
  va_start (ap, iconname);
  for (;;)
    {
      char *btn_lbl, *btn_icon;

      btn_lbl = va_arg (ap, char *);
      if (btn_lbl == NULL)
	break;
      
      btn_icon = va_arg (ap, char *);
      
      add_button (btn_lbl, btn_icon, window, buttonhbox, i++);
    }
  va_end (ap);

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
