/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>

#include <gtk/gtk.h>

#include "pixmaps.h"
#include "init.h"
#include "render.h"
#include "picturebutton.h"

static struct pix my_pix[] = {
  { "keys", "keys" },
  { NULL, NULL }
};

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { NULL, NULL }
};

#define _(x) gettext(x)

int
main(int argc, char *argv[])
{
  struct pix *p;
  GtkWidget *window;
  GtkWidget *hbox, *vbox;
  GtkWidget *entry, *label;
  GtkWidget *buttonok, *buttoncancel;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_pixmaps (my_pix) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_dialog_new ();

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  entry = gtk_entry_new ();
  gtk_widget_show (entry);
  label = gtk_label_new (_("Enter password:"));
  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  p = gpe_find_pixmap ("keys");
  if (p)
    {
      GtkWidget *pw = gtk_pixmap_new (p->pixmap, p->mask);
      gtk_widget_show (pw);
      gtk_box_pack_start (GTK_BOX (hbox), pw, FALSE, FALSE, 4);
    }

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
		      hbox, TRUE, FALSE, 0);

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("OK"), "ok");
  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
		      buttonok, TRUE, TRUE, 0);

  gtk_widget_show (window);

  gtk_widget_grab_focus (entry);

  gtk_main ();

  exit (0);
}
