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
#include <libintl.h>

#include <gpe/spacing.h>

#include "db.h"
#include "structure.h"


#define _(x) gettext(x)

gboolean
do_edit_name_detail(GtkWindow *parent, struct person *p)
{
  GtkWidget *dialog, *lgiven, *lfamily, *lsuffix, *ltitle;
  GtkWidget *table, *egiven, *efamily, *esuffix, *etitle;
  GtkWidget *btnOK;
  
  struct tag_value *v;
    
  /* create dialog window */
  dialog = gtk_dialog_new_with_buttons(_("Edit Name Details"),parent,
                                       GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       NULL);
  btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog),GTK_STOCK_OK, 
                                GTK_RESPONSE_OK);
  GTK_WIDGET_SET_FLAGS(btnOK,GTK_CAN_DEFAULT);
  gtk_widget_grab_default(btnOK);
  gtk_widget_grab_focus(btnOK);
  
  /* supply action area */
  
  egiven = gtk_entry_new();
  efamily = gtk_entry_new();
  esuffix = gtk_entry_new();
  etitle = gtk_entry_new();
  
  lgiven = gtk_label_new(_("Given name:"));
  lfamily = gtk_label_new(_("Family name:"));
  lsuffix = gtk_label_new(_("Suffix:"));
  ltitle = gtk_label_new(_("Title:"));
  
  gtk_misc_set_alignment(GTK_MISC(lgiven),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(lfamily),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(lsuffix),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(ltitle),1,0.5);
  
  table = gtk_table_new(4,2,FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table),gpe_get_border());
  gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  
  gtk_table_attach(GTK_TABLE(table),ltitle,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lgiven,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lfamily,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lsuffix,0,1,3,4,GTK_FILL,GTK_FILL,0,0);
  
  gtk_table_attach(GTK_TABLE(table),etitle,1,2,0,1,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),egiven,1,2,1,2,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),efamily,1,2,2,3,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),esuffix,1,2,3,4,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);

  /* fill in values */
  v = db_find_tag(p,"TITLE");
  if (v) gtk_entry_set_text(GTK_ENTRY(etitle),v->value);
  v = db_find_tag(p,"GIVEN_NAME");
  if (v) gtk_entry_set_text(GTK_ENTRY(egiven),v->value);
  v = db_find_tag(p,"FAMILY_NAME");
  if (v) gtk_entry_set_text(GTK_ENTRY(efamily),v->value);
  v = db_find_tag(p,"HONORIFIC_SUFFIX");
  if (v) gtk_entry_set_text(GTK_ENTRY(esuffix),v->value);

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),table,TRUE,TRUE,0);
  gtk_widget_show_all(dialog);
  
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
  {
    /* save values */
    db_set_data(p, "TITLE", g_strdup(gtk_entry_get_text(GTK_ENTRY(etitle))));
    db_set_data(p, "GIVEN_NAME", g_strdup(gtk_entry_get_text(GTK_ENTRY(egiven))));
    db_set_data(p, "FAMILY_NAME", g_strdup(gtk_entry_get_text(GTK_ENTRY(efamily))));
    db_set_data(p, "HONORIFIC_SUFFIX", g_strdup(gtk_entry_get_text(GTK_ENTRY(esuffix))));
    db_set_data(p, "NAME", g_strdup(""));
    update_edit(p, parent);
    gtk_widget_destroy(dialog);
    return TRUE;
  }
  gtk_widget_destroy(dialog);
  return FALSE;
}
