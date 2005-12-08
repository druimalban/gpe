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

#include <gpe/contacts-db.h>
#include "structure.h"
#include "namedetail.h"

#define _(x) gettext(x)
#define N_(z) (z)

/* TRANSLATORS: These abbreviations are a list of common titles used in a name */
gchar *titles[] = {N_("Dr."), N_("Miss"), N_("Mr."), N_("Mrs."), N_("Ms."), N_("Prof."), N_("Sir"), NULL};
/* TRANSLATORS: This is a list of common name suffixes */
gchar *suffixes[] = {N_("Jr."), N_("Esq."), N_("Sr."), N_("I"), N_("II"), N_("III"), NULL};

gboolean
do_edit_name_detail(GtkWindow *parent, struct contacts_person *p)
{
  GtkWidget *dialog, *lgiven, *lfamily, *lsuffix, *ltitle, *lmiddle;
  GtkWidget *table, *egiven, *efamily, *esuffix, *etitle, *emiddle;
  GtkWidget *cbTitle, *cbSuffix;
  GtkWidget *btnOK;
  GList *l_suffixes = NULL, *l_titles = NULL;
  int i = 0;
  
  struct contacts_tag_value *v;
  
  /* build lists */  
  
  while (suffixes[i])
    {
      l_suffixes = g_list_append(l_suffixes, gettext(suffixes[i]));
      i++;
    }
  i = 0; 
  while (titles[i])
    {
      l_titles = g_list_append(l_titles, gettext(titles[i]));
      i++;
    }
    
  /* create dialog window */
#ifdef IS_HILDON
  dialog = gtk_dialog_new_with_buttons(_("Edit Name Details"), parent, 
                                       GTK_DIALOG_MODAL, NULL);
  btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog), _("OK"), 
                                GTK_RESPONSE_OK);
  gtk_dialog_add_button(GTK_DIALOG(dialog), _("Cancel"), 
                        GTK_RESPONSE_CANCEL);
#else    
  dialog = gtk_dialog_new_with_buttons(_("Edit Name Details"), parent,
                                       GTK_DIALOG_MODAL,
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       NULL);
  btnOK = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_OK, 
                                GTK_RESPONSE_OK);
#endif
  GTK_WIDGET_SET_FLAGS(btnOK, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(btnOK);
  gtk_widget_grab_focus(btnOK);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
  
  /* supply action area */
  
  cbTitle = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(cbTitle), FALSE, TRUE);
  gtk_combo_set_popdown_strings(GTK_COMBO(cbTitle), l_titles);
  
  cbSuffix = gtk_combo_new();
  gtk_combo_set_value_in_list(GTK_COMBO(cbSuffix), FALSE, TRUE);
  gtk_combo_set_popdown_strings(GTK_COMBO(cbSuffix), l_suffixes);
  
  egiven = gtk_entry_new();
  emiddle = gtk_entry_new();
  efamily = gtk_entry_new();
  esuffix = GTK_COMBO(cbSuffix)->entry;
  etitle = GTK_COMBO(cbTitle)->entry;
  
  lgiven = gtk_label_new(_("First name:"));
  lmiddle = gtk_label_new(_("Middle name:"));
  lfamily = gtk_label_new(_("Last name:"));
  lsuffix = gtk_label_new(_("Suffix:"));
  ltitle = gtk_label_new(_("Title:"));
  
  gtk_misc_set_alignment(GTK_MISC(lgiven),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(lfamily),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(lmiddle),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(lsuffix),1,0.5);
  gtk_misc_set_alignment(GTK_MISC(ltitle),1,0.5);
  
  table = gtk_table_new(4,2,FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(table),gpe_get_border());
  gtk_table_set_col_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  gtk_table_set_row_spacings(GTK_TABLE(table),gpe_get_boxspacing());
  
  gtk_table_attach(GTK_TABLE(table),ltitle,0,1,0,1,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lgiven,0,1,1,2,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lmiddle,0,1,2,3,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lfamily,0,1,3,4,GTK_FILL,GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),lsuffix,0,1,4,5,GTK_FILL,GTK_FILL,0,0);
  
  gtk_table_attach(GTK_TABLE(table),cbTitle,1,2,0,1,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),egiven,1,2,1,2,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),emiddle,1,2,2,3,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),efamily,1,2,3,4,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);
  gtk_table_attach(GTK_TABLE(table),cbSuffix,1,2,4,5,GTK_FILL | GTK_EXPAND,
                   GTK_FILL,0,0);

  /* fill in values */
  v = contacts_db_find_tag(p,"TITLE");
  if (v && v->value) 
    gtk_entry_set_text(GTK_ENTRY(etitle),v->value);
  else
    gtk_entry_set_text(GTK_ENTRY(etitle),"");
  v = contacts_db_find_tag(p,"GIVEN_NAME");
  if (v && v->value) gtk_entry_set_text(GTK_ENTRY(egiven),v->value);
  v = contacts_db_find_tag(p,"MIDDLE_NAME");
  if (v && v->value) gtk_entry_set_text(GTK_ENTRY(emiddle),v->value);
  v = contacts_db_find_tag(p,"FAMILY_NAME");
  if (v && v->value) gtk_entry_set_text(GTK_ENTRY(efamily),v->value);
  v = contacts_db_find_tag(p,"HONORIFIC_SUFFIX");
  if (v && v->value) 
    gtk_entry_set_text(GTK_ENTRY(esuffix),v->value);
  else
    gtk_entry_set_text(GTK_ENTRY(esuffix),"");

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),table,TRUE,TRUE,0);
  gtk_widget_show_all(dialog);
  
  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
      /* save values */
      contacts_db_set_data(p, "TITLE", g_strdup(gtk_entry_get_text(GTK_ENTRY(etitle))));
      contacts_db_set_data(p, "GIVEN_NAME", g_strdup(gtk_entry_get_text(GTK_ENTRY(egiven))));
      contacts_db_set_data(p, "MIDDLE_NAME", g_strdup(gtk_entry_get_text(GTK_ENTRY(emiddle))));
      contacts_db_set_data(p, "FAMILY_NAME", g_strdup(gtk_entry_get_text(GTK_ENTRY(efamily))));
      contacts_db_set_data(p, "HONORIFIC_SUFFIX", g_strdup(gtk_entry_get_text(GTK_ENTRY(esuffix))));
      contacts_db_set_data(p, "NAME", g_strdup(""));
      gtk_widget_destroy(dialog);
      g_list_free(l_titles);
      g_list_free(l_suffixes);
      return TRUE;
    }
  gtk_widget_destroy(dialog);
  g_list_free(l_titles);
  g_list_free(l_suffixes);

  return FALSE;
}
