#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>

#include "../include/callbacks.h"
#include "interface.h"
#include "support.h"
#include "db.h"
#include "../include/questionfx.h"

extern GtkWidget *wdcmain;
static t_sql_handle *db = NULL;


void
list_toggle_read (GtkCellRendererToggle * cellrenderertoggle,
		  gchar * path_str, gpointer model_data)
{
  GtkTreeModel *model = (GtkTreeModel *) model_data;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean ac_read;
  gboolean ac_write;
  GtkWidget *edit;
  gchar *username;
  struct passwd *pwdinfo;
  uint perms = 0;
  gchar *tabname;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_READ, &ac_read, -1);

  /* do something with the value */
  ac_read ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_READ, ac_read,
		      -1);
  gtk_tree_model_get (model, &iter, COLUMN_WRITE, &ac_write, -1);
  gtk_tree_model_get (model, &iter, COLUMN_USER, &username, -1);
  edit = lookup_widget (GTK_WIDGET (wdcmain), "eTable");
  pwdinfo = getpwnam (username);
  if (pwdinfo != NULL)
    {
      if (ac_read)
	perms = AC_READ;
      if (ac_write)
	perms += AC_WRITE;
      tabname = gtk_editable_get_chars (GTK_EDITABLE (edit), 0, -1);
      gpe_acontrol_set_table (db, tabname, pwdinfo->pw_uid, perms);
    }
  /* clean up */
  gtk_tree_path_free (path);
}


void
list_toggle_write (GtkCellRendererToggle * cellrenderertoggle,
		   gchar * path_str, gpointer model_data)
{
  GtkTreeModel *model = (GtkTreeModel *) model_data;
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean ac_read;
  gboolean ac_write;
  GtkWidget *edit;
  gchar *username;
  struct passwd *pwdinfo;
  uint perms = 0;
  gchar *tabname;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_WRITE, &ac_write, -1);

  /* do something with the value */
  ac_write ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_WRITE, ac_write,
		      -1);
  gtk_tree_model_get (model, &iter, COLUMN_READ, &ac_read, -1);
  gtk_tree_model_get (model, &iter, COLUMN_USER, &username, -1);
  edit = lookup_widget (GTK_WIDGET (wdcmain), "eTable");
  pwdinfo = getpwnam (username);
  if (pwdinfo != NULL)
    {
      if (ac_read)
	perms = AC_READ;
      if (ac_write)
	perms += AC_WRITE;
      tabname = gtk_editable_get_chars (GTK_EDITABLE (edit), 0, -1);
      gpe_acontrol_set_table (db, tabname, pwdinfo->pw_uid, perms);
    }
  /* clean up */
  gtk_tree_path_free (path);
}


static int
rule_cb (void *arg, int argc, char **argv, char **data)
{
  struct passwd *pwdinfo;

  GtkListStore *liststore;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  uint ac_read, ac_write;

  if (argc > 0 && argv[0])
    {
      treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
      liststore = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
      gtk_list_store_append (liststore, &iter);
      pwdinfo = getpwuid (atoi (argv[1]));
      if (pwdinfo)
	{
	  switch (atoi (argv[2]))
	    {
	    case AC_READ:
	      ac_read = 1;
	      ac_write = 0;
	      break;
	    case AC_WRITE:
	      ac_read = 0;
	      ac_write = 1;
	      break;
	    case AC_READWRITE:
	      ac_read = 1;
	      ac_write = 1;
	      break;
	    default:
	      ac_read = 0;
	      ac_write = 0;
	      break;
	    }

	  gtk_list_store_set (liststore, &iter, COLUMN_USER, pwdinfo->pw_name,
			      COLUMN_READ, ac_read, COLUMN_WRITE, ac_write,
			      NUM_COLUMNS, 00001, -1);
	}
    }

  return 0;
}


void
on_wdcmain_show (GtkWidget * widget, gpointer user_data)
{
  GList *items = NULL;
  GtkWidget *combo;

  combo = lookup_widget (widget, "cbDatabase");
  items = g_list_append (items, "contacts");
  items = g_list_append (items, "todo");
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);

}

void
on_wdcmain_destroy (GtkObject * object, gpointer user_data)
{
  if (db)
    gpe_acontrol_db_close (db);
}


void
on_combo_entry2_changed (GtkEditable * editable, gpointer user_data)
{
  GList *items = NULL;
  GtkWidget *combo;
  int cnt, i;
  char **tables;
  gchar *etext;

  etext = gtk_editable_get_chars (editable, 0, -1);
  if (!strlen (etext))
    return;

  if (db)
    gpe_acontrol_db_close (db);
  db = gpe_acontrol_db_open (etext);
  if (db)
    {
      items = NULL;
      cnt = gpe_acontrol_list_tables (db, &tables);
      for (i = 1; i < cnt; i++)	// first is field name
	items = g_list_append (items, tables[i]);
      combo = lookup_widget (GTK_WIDGET (editable), "cbTable");
      gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
      sql_free_table (tables);
    }

}


void
on_combo_entry1_changed (GtkEditable * editable, gpointer user_data)
{
  GtkListStore *liststore;
  GtkTreeView *treeview;
  gchar *etext;

  etext = gtk_editable_get_chars (editable, 0, -1);
  if (!strlen (etext))
    return;

  treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
  liststore = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (liststore);
  gpe_acontrol_get_list (db, etext, rule_cb);
}


void
on_bNew_clicked (GtkButton * button, gpointer user_data)
{
  gchar *username;
  gchar *tabname;
  struct passwd *pwdinfo;

  GtkListStore *liststore;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  GtkWidget *edit;


  gpe_question_fx_ask ("Username", "Question", "icon", &username, _("Ok"),
		       "ok", _("Cancel"), "cancel", NULL);
  treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
  liststore = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));

  pwdinfo = getpwnam (username);
  if (pwdinfo)
    {
      gtk_list_store_append (liststore, &iter);
      gtk_list_store_set (liststore, &iter, COLUMN_USER, pwdinfo->pw_name,
			  COLUMN_READ, 0, COLUMN_WRITE, 0, NUM_COLUMNS, 00001,
			  -1);
      edit = lookup_widget (GTK_WIDGET (wdcmain), "eTable");

      tabname = gtk_editable_get_chars (GTK_EDITABLE (edit), 0, -1);
      gpe_acontrol_set_table (db, tabname, pwdinfo->pw_uid, AC_NONE);

    }
  else
    gpe_error_box (_("This user doesn 't exist!"));

}


void
on_bDelete_clicked (GtkButton * button, gpointer user_data)
{
  gchar *username;
  gchar *tabname;
  struct passwd *pwdinfo;

  GtkTreeModel *liststore;
  GtkTreeSelection *selection;
  GtkTreeView *treeview;
  GtkTreeIter iter;
  GtkWidget *edit;


  if (gpe_question_ask_yn (_("Really delete rule?")))
    {
      treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
      liststore = GTK_TREE_MODEL (gtk_tree_view_get_model (treeview));
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      if (gtk_tree_selection_get_selected (selection, &liststore, &iter))
	{
	  gtk_tree_model_get (liststore, &iter, COLUMN_USER, &username, -1);
	}

      pwdinfo = getpwnam (username);
      if (pwdinfo)
	{
	  gtk_list_store_remove (GTK_LIST_STORE (liststore), &iter);

	  edit = lookup_widget (GTK_WIDGET (wdcmain), "eTable");

	  tabname = gtk_editable_get_chars (GTK_EDITABLE (edit), 0, -1);
	   gpe_acontrol_remove_rule(db, tabname, pwdinfo->pw_uid);
	}

    }
}
