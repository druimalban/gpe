#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe-sql.h>
#include <string.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "db.h"
#include "questionfx.h"
#include "usqld-cfgdb.h"

extern GtkWidget *wdcmain;
extern GtkWidget *bNew;
extern GtkWidget *bDelete;

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

  if (db == NULL)
    return;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_READ, &ac_read, -1);

  /* do something with the value */
  ac_read ^= 1;

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
      if (gpe_acontrol_set_table (db, tabname, pwdinfo->pw_uid, perms) >= 0)
	{
	  /* set new value */
	  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_READ,
			      ac_read, -1);

	}
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

  if (db == NULL)
    return;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_WRITE, &ac_write, -1);

  /* do something with the value */
  ac_write ^= 1;

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
      if (gpe_acontrol_set_table (db, tabname, pwdinfo->pw_uid, perms) >= 0)
	{
	  /* set new value */
	  gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_WRITE,
			      ac_write, -1);
	}
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

  if (argc > 1 && argv[0])
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
  GtkWidget *combo, *cbedit;
  gchar *content = NULL;
  uint avalue = AC_NONE;

  combo = lookup_widget (widget, "cbDatabase");
  items = g_list_append (items, "contacts");
  items = g_list_append (items, "todo");
  gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);

  if (!set_config_open (TRUE))
	  fprintf(stderr,"err: could't open config\n");

  combo = lookup_widget (widget, "cbBaseDir");
  cbedit = lookup_widget (widget, "eBaseDir");
  content = get_config_val(TAG_CFG_BASEDIR);
  if (content)
  {
    gtk_entry_set_text(GTK_ENTRY(cbedit),content);
  }	
  combo = lookup_widget (widget, "cbPolicy");
  if (!set_config_open (TRUE))
	  fprintf(stderr,"err: could't open config\n");
  content = get_config_val(TAG_CFG_GLOBALPOLICY);
  if (content)
    {
      avalue = atoi(content);
      free (content);
      switch (avalue){
		  case AC_NONE: gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list),0);
		  break;
		  case AC_READ: gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list),1);
		  break;
		  case AC_READWRITE: gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list),2);
		  break;
		  default: gtk_list_select_item(GTK_LIST(GTK_COMBO(combo)->list),0);
	  }
    }
	
  set_config_open (FALSE);

}

void
on_wdcmain_destroy (GtkObject * object, gpointer user_data)
{
  if (db)
    sql_close (db);
}

// changed database selection
void
on_combo_entry2_changed (GtkEditable * editable, gpointer user_data)
{
  GList *items = NULL;
  GtkWidget *combo;
  int cnt, i;
  char **tables;
  gchar *etext;
  GtkListStore *liststore;
  GtkTreeView *treeview;

  etext = gtk_editable_get_chars (editable, 0, -1);
  if (!strlen (etext))
    return;
  items = NULL;
  combo = lookup_widget (GTK_WIDGET (editable), "cbTable");
  treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
  liststore = GTK_LIST_STORE (gtk_tree_view_get_model (treeview));
  gtk_list_store_clear (liststore);
  if (db)
    sql_close (db);
  db = sql_open (etext);
  if (db)
    {
      cnt = sql_list_tables (db, &tables);
      for (i = 1; i < cnt; i++)	// first is field name
	{
	  items = g_list_append (items, tables[i]);
	}
      if (items != NULL)
	{
	  gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
	  gtk_widget_set_sensitive (combo, TRUE);
	  gtk_widget_set_sensitive (GTK_WIDGET (treeview), TRUE);
	}
      else
	{
	  items = g_list_append (items, _("<none>"));
	  gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
	  gtk_widget_set_sensitive (combo, FALSE);
	  gtk_widget_set_sensitive (GTK_WIDGET (treeview), FALSE);
	}
      sql_free_table (tables);
    }
  else
    {
      gtk_widget_set_sensitive (combo, FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (treeview), FALSE);
    }
}


// changed table selection
void
on_combo_entry1_changed (GtkEditable * editable, gpointer user_data)
{
  GtkListStore *liststore;
  GtkTreeView *treeview;
  gchar *etext;
  if (db == NULL)
    return;
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
	  gpe_acontrol_remove_rule (db, tabname, pwdinfo->pw_uid);
	}

    }
}

void
on_eBaseDir_editing_done (GtkEditable * editable, gpointer user_data)
{
  gchar *content = NULL;
  // do some checks here
  content = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);
  if (content)
    {
      set_config_open (TRUE);
      set_config_val (TAG_CFG_BASEDIR, content);
      set_config_open (FALSE);
    }
}


void
on_ePolicy_editing_done (GtkEditable * editable, gpointer user_data)
{
  gchar *content = NULL;
  uint avalue = AC_NONE;

  // this needs some improvement
  content = gtk_editable_get_chars (GTK_EDITABLE (editable), 0, -1);

  if (content)
    {
      if (!strcmp (content, "allow read"))
	avalue = AC_READ;
      if (!strcmp (content, "allow read and write"))
	avalue = AC_READWRITE;
      sprintf (content, "%i", avalue);
      set_config_open (TRUE);
      set_config_val (TAG_CFG_GLOBALPOLICY, content);
      free (content);
      set_config_open (FALSE);
    }

}

void
on_eUserdir_changed                    (GtkEditable     *editable,
                                        gpointer         user_data)
{

}

