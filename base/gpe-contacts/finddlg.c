/*
 * Copyright (C) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This file is part of gpe-contacts.
 * Module: Find dialog.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>

#include <gpe/spacing.h>

#include "db.h"
#include "finddlg.h"

#define _(x) gettext(x)


GSList*
do_find_contacts(GtkWindow *parent)
{
  GtkWidget *dialog, *label;
  GtkWidget *entry;
  GtkWidget *btnOK;
  GSList *result = NULL;
    
  /* create dialog window */
  dialog = gtk_dialog_new_with_buttons(_("Find Contacts"), parent,
                                       GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       NULL);

  btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, 
                                GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(btnOK, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(btnOK);
  
  /* supply action area */
  
  label = gtk_label_new(_("Search for:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  
  entry = gtk_entry_new();
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
  
  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 
                                 gpe_get_border());
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog)->vbox), gpe_get_boxspacing());  
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry, FALSE, TRUE, 0);

  gtk_widget_show_all(dialog);
  
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
      result = db_get_entries_filtered(gtk_entry_get_text(GTK_ENTRY(entry)));
    }
  gtk_widget_destroy(dialog);
  return result;
}
