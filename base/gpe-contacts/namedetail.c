/*
 * Copyright (C) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This file is part of gpe-contacts.
 * Module: Contact name detail editor.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <gpe/spacing.h>

#include "structure.h"

gboolean
do_edit_name_detail(GtkWindow *parent, struct person *p)
{
  GtkWidget *dialog, *label;
  GtkWidget *egiven, *efamily, *eprefix, *esuffix, *etitle;
  
  /* create dialog window */
  dialog = gtk_dialog_new_with_buttons(_("Edit Name Details"),parent,
                                       GTK_DIALOG_MODAL,GTK_STOCK_ABORT,
                                       GTK_STOCK_OK,NULL);
  
  /* supply action area */
  
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
  {
    /* save values */
    
    gtk_widget_destroy(dialog);
    return TURE;
  }
  gtk_widget_destroy(dialog);
  return FALSE;
}
