/*
 * This file is part of gpe-timeheet
 * (c) 2004, 2006 Florian Boor <florian.boor@kernelconcepts.de>
 * (c) 2005 Philippe De Swert <philippedeswert@scarlet.be>
 * (c) 2006 Michele Giorgini <md6604@mclink.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 *
 * here you'll find useful procedures for the ui
*/

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>

/* gdk-gtk-gpe includes */
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gpehelp.h>
#include <gpe/gpedialog.h>
#include <gpe/gtkdatecombo.h>

/* timesheet includes */
#include "sql.h"
#include "journal.h"
#include "ui.h"

#define _(_x) gettext(_x)

char *appname="gpe-timesheet";
static GtkWidget *btn_con, *btn_coff;

GtkWidget *main_appview;

/* first some procedures not strictly correlated to ui
   but regarding the inner core functions of the program */

gboolean 
stop_timing_all_cb  (GtkTreeModel *model,
                    GtkTreePath   *path,
                    GtkTreeIter   *iter,
                    gpointer      user_data)
{ /*  stop any clocked in task 
  *   not efficient, but works */

  struct task *t;
  t = g_malloc (sizeof (struct task));

  gtk_tree_model_get (model, iter, ID, &t->id, STARTED, &t->started, -1);

  if (t->started)
  {
    log_entry (STOP, time (NULL), t, "timesheet closing");
    gtk_tree_store_set (GTK_TREE_STORE(model), iter, STATUS, NULL, STARTED, FALSE, ICON_STARTED, NULL, -1);
  }

  g_free(t);

  return FALSE;
}

static void
show_help (void)
{ /* show help */
 gboolean test;
 char *topic = NULL;
 
 test = gpe_show_help(appname, topic);
 if (test == TRUE)
 	gpe_error_box (_("Help not (or incorrectly) installed. Or no helpviewer application registered."));
}

static gboolean
exit_app (GtkWidget *w, GdkEvent *evt, gpointer user_data)
{ /* exits the application */
  GtkTreeModel *model = gtk_tree_view_get_model(user_data);

  gtk_tree_model_foreach(model,stop_timing_all_cb,NULL);

  return FALSE;
}

static void
set_active (int start, int stop) 
{ /* this toggle the status of the time_in and time_out buttons */
  if (start)
    gtk_widget_set_sensitive (btn_con, TRUE);
  else
    gtk_widget_set_sensitive (btn_con, FALSE);

  if (stop)
    gtk_widget_set_sensitive (btn_coff, TRUE);
  else
    gtk_widget_set_sensitive (btn_coff, FALSE);
}

static void
confirm_click_ok (GtkWidget *widget, gpointer p)
{
  gtk_main_quit ();
}

static void
confirm_click_cancel (GtkWidget *widget, gpointer p)
{
  gtk_widget_destroy (GTK_WIDGET (p));
}

static void
confirm_note_destruction (GtkWidget *widget, gpointer p)
{ /* this ends the insertion of a new note */
  gboolean *b = (gboolean *)p;

  if (*b == FALSE)
    {
      *b = TRUE;

      gtk_main_quit ();
    }
}

static gboolean
confirm_dialog (gchar **text, gchar *action, gchar *action2)
{ /* dialog box for clocking in/out */
  GtkWidget *w = gtk_dialog_new ();
  gboolean destroyed = FALSE;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *pixbuf;
  GtkWidget *icon;
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  GtkWidget *action_label, *action2_label;
  GtkWidget *frame, *entry;

  gtk_window_set_title (GTK_WINDOW (w), _("Time Tracker"));

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);

  if (action)
    {
      action_label = gtk_label_new (action);
      gtk_box_pack_start (GTK_BOX (vbox), action_label, FALSE, FALSE, 0);
      gtk_widget_show (action_label);
      gtk_misc_set_alignment (GTK_MISC (action_label), 0.0, 0.5);
    }
  action2_label = gtk_label_new (action2);
  gtk_box_pack_start (GTK_BOX (vbox), action2_label, FALSE, FALSE, 0);
  gtk_widget_show (action2_label);

  gtk_misc_set_alignment (GTK_MISC (action2_label), 0.0, 0.5);
  
  gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 8);

  pixbuf = gpe_find_icon ("gpe-timesheet");
  if (pixbuf)
    {
      icon = gtk_image_new_from_pixbuf (pixbuf);
      gtk_box_pack_end (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
      gtk_widget_show (icon);
    }

  frame = gtk_frame_new (_("Notes"));
  gtk_widget_show (frame);
  entry = gtk_text_view_new ();
  gtk_widget_show (entry);
  gtk_container_add (GTK_CONTAINER (frame), entry);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox), frame, TRUE, TRUE, 0);

  buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
  buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->action_area), 
		      buttonok, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (buttonok), "clicked", 
		      G_CALLBACK (confirm_click_ok), NULL);
  g_signal_connect (G_OBJECT (buttoncancel), "clicked", 
		      G_CALLBACK (confirm_click_cancel), w);

  g_signal_connect (G_OBJECT (w), "destroy",
		      G_CALLBACK(confirm_note_destruction), &destroyed);

  gtk_window_set_modal (GTK_WINDOW (w), TRUE);
  gtk_widget_show_all (w);
  gtk_dialog_set_has_separator (GTK_DIALOG (w), FALSE);

  gtk_widget_grab_focus (GTK_WIDGET (entry));

  gtk_main ();

  if (destroyed)
    return FALSE;
  {
    GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (entry));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (buf, &start, &end);
    *text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
  }

  destroyed = TRUE;
  gtk_widget_destroy (w);

  return TRUE;
}

static void
start_timing (GtkWidget *w, gpointer user_data)
{ /* starts the timing of a task */
  GtkTreeSelection  *selection = GTK_TREE_SELECTION(user_data);
  GtkTreeModel      *model;
  GtkTreeIter       iter;
  GdkPixbuf         *icon;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    struct task *t;
    gchar *text = " ";
    t = g_malloc (sizeof (struct task));

    gtk_tree_model_get (model, &iter, ID, &t->id, DESCRIPTION, &t->description, -1);

    if (t) 
      {
        if (confirm_dialog (&text, _("Clocking in:"), t->description))
        {
          icon = gpe_find_icon("tick");
          gtk_tree_store_set (GTK_TREE_STORE(model), &iter, ICON_STARTED, icon, STARTED, TRUE, STATUS, "...in progress...", -1);
          t->started = TRUE;
          /* mark_started (ct,mode) */
          log_entry (START, time (NULL), t, text);
          set_active (0,1);
        }
      }
      g_free(t);
    }
}

static void
stop_timing (GtkWidget *w, gpointer user_data)
{ /* stop the timing of a task */
  GtkTreeSelection *selection = GTK_TREE_SELECTION(user_data);
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    struct task *t;
    gchar *text = " ";
    t = g_malloc (sizeof (struct task));
    gtk_tree_model_get (model, &iter, ID, &t->id, DESCRIPTION, &t->description, -1);
  
    if (t) 
      {
        if (confirm_dialog (&text, _("Clocking out:"), t->description))
        {
          gtk_tree_store_set (GTK_TREE_STORE(model), &iter, STATUS, NULL, STARTED, FALSE, ICON_STARTED, NULL, -1);
          t->started = FALSE;
          //mark_started (ct,mode)
          log_entry (STOP, time (NULL), t, text);
          set_active (1,0);
        }
      }
      g_free(t);
    }
}

static journal_changed_row_cb (GtkWidget *selection, gpointer user_data)
{
  GtkWidget *edit_button = user_data;

  GtkTreeModel *model;
  GtkTreeIter iter;
  guint idx;

  if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION(selection), &model, &iter))
    {
      gtk_tree_model_get (model, &iter, ID, &idx, -1);
      if (idx<1)
        gtk_widget_set_sensitive (edit_button, FALSE);
      else
        gtk_widget_set_sensitive (edit_button, TRUE);
    }
}

static void
ui_delete_task (GtkWidget *w, gpointer user_data)
{ /* deletes a task from the GtkTreeStore */
  GtkTreeSelection  *selection = user_data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  //GtkTreeModelFilter *filter;
  GtkTreeStore *store;
  int idx;

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, ID, &idx, -1);
      if (idx)
        { /* this is to ensure that we don't try to delete tasks
          ** using todo's indexes, which could lead to disasters... */
          delete_children (idx);
          delete_task (idx);
        /* before deleting the task from the treestore we must be sure that
            the database deletions have been performed! code to do */
          gtk_tree_store_remove (GTK_TREE_STORE(model), &iter);
        }
    }
}

static void
ui_new_task (GtkWidget *w, gpointer p)
{ /* directly insert a new task in the GtkTreeStore */
  GtkTreeStore *task_store = GTK_TREE_STORE (p);
  GtkTreeIter iter;

  gchar *text[2];
  guint pt = 0;
  text[0] = smallbox (_("New task"), _("Name"), "");
  text[1] = "";
  if (text[0])
    {
      struct task *t;
      uint idx;
      t = new_task (text[0], pt, 0);
      if (t != NULL)
      {
        idx = t->id;
        gtk_tree_store_append(task_store, &iter, NULL);
        gtk_tree_store_set (task_store, &iter, ID, idx, DESCRIPTION, text[0], STATUS, text[1], -1);
      }
    }
}

static void
ui_new_sub_task (GtkWidget *w, gpointer p)
{ /* insert a new sub_task in the GtkTreeStore */
  GtkTreeSelection  *selection = p;
  GtkTreeIter parent, iter;
  GtkTreeModel *model;

  gchar *text[2];
  guint task_id, todo_id;

  if (gtk_tree_selection_get_selected (selection, &model, &parent))
    { /* it means that an item is selected and we can create a subtask */
      gtk_tree_model_get (model, &parent, ID, &task_id, TODO_ID, &todo_id, -1);
      /* first we check if the current item already has logs
      ** in that case we don't create a new subtask
      ** in the future, every log should be moved to the subtask
      ** just created 
      ** if id==0 it means that it is a root todo item and subsequently
      ** it can always have children */
      if (!check_if_item_has_logs(task_id) || task_id==0)
        {
          text[0] = smallbox (_("New sub-task"), _("Name"), "");
          text[1] = "";
          if (text[0])
            {
              struct task *t;
              uint idx;

              t = new_task (text[0], task_id, todo_id);
              if (t != NULL)
                {
                  idx = t->id;
                  gtk_tree_store_append(GTK_TREE_STORE(model), &iter, &parent);
                  gtk_tree_store_set (GTK_TREE_STORE(model), &iter, ID, idx, DESCRIPTION, text[0], STATUS, text[1], -1);
                  /* after the insertion of a new sub task
                  ** we have to refresh the status of clock_in and clock_out buttons
                  ** since a task with subtasks couldn't be timed! */
                  set_active (0,0);
                }
            }
        }
      else
        {
          gpe_error_box(_("You cannot create a subtask of an already logged item!"));
        }
    }
}

static void
view_selected_row_cb (GtkTreeSelection *selection, gpointer data)
{ /* callback function that updates clock_in and clock_out buttons when changing row */
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *str1, *str2;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
  {
    if (gtk_tree_model_iter_has_child (model, &iter))
      set_active (0,0);
    else
      { struct task *t;
        int is_not_todo;
        str2="...in progress...";

        t = g_malloc (sizeof (struct task));
        gtk_tree_model_get (model, &iter, STATUS, &str1, ID, &is_not_todo, -1);

        if (!is_not_todo)
          {
            set_active(0,0);
          }
        else
          { if (str1 != NULL)
              {
                if (!strncmp(str1,str2,1))
                    {
                      set_active (0,1);
                    }
                else
                  {
                    set_active (1,0);
                  }
              }
            else
              set_active (1,0);
          }
          g_free(t);
      }
  }
}

#ifdef IS_HILDON

static void
toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data)
{ /* this one switchs on/off the main toolbar */
  GtkWidget *toolbar = user_data;

  if (gtk_check_menu_item_get_active(menuitem))
    gtk_widget_show(toolbar);
  else
    gtk_widget_hide(toolbar);
}

static void
toggle_journal(GtkCheckMenuItem *menuitem, gpointer user_data)
{ /* this one switchs on/off the journal view */
  GtkWidget *treeview = user_data;

  if (gtk_check_menu_item_get_active(menuitem))
    gtk_widget_show(treeview);
  else
    gtk_widget_hide(treeview);
}

static void 
filter_show_task(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gtk_tree_store_clear(global_task_store);
  sql_start();
}

static void
filter_show_todo(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gtk_tree_store_clear(global_task_store);
  sql_append_todo();
}

static void
filter_show_both(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  gtk_tree_store_clear(global_task_store);
  sql_start();
  sql_append_todo();
}

#endif

void 
refresh_list(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  GtkWidget *view_task_and_todo;
  view_task_and_todo = user_data;
  gtk_tree_store_clear(global_task_store);
  sql_start();
  sql_append_todo();
  if (view_task_and_todo)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (view_task_and_todo), TRUE);
}

/* and last the procedure that creates the main interface */

GtkWidget * create_interface(GtkWidget *main_window)
{  /* this is the procedure that is going to be executed at startup and loads data
      from database into the global_task_store structure */
  GtkWidget *main_vbox, *main_hpaned, *journal_vbox;
  GtkWidget *journal_scrolled_window, *main_scrolled_window;
  GtkWidget *main_toolbar, *journal_toolbar;
  GtkWidget *pw;
  GtkWidget *w;
  GtkWidget *task_view, *journal_view;
  GtkTreeStore *task_store;
  GtkTreeModel *model;
  GtkTreeViewColumn *col, *col_journal;
  GtkTreeSelection *task_selection, *journal_selection;
  GtkCellRenderer *main_renderer[3], *renderer[4];
  GdkPixbuf *p;
  GtkToolItem *new, *new_sub, *delete, *refresh, *edit, *clock_in, *clock_out, *show, *html, *quit, *back;
  GtkToolItem *sep, *sep1, *sep2, *sep3;
#ifdef IS_HILDON
  GtkWidget *rframe;
#endif

/*  about treestore and treeview definition:
    global_task_store is a pointer at the extern global structure
    where tasks are retrieved */

  task_store = global_task_store;
  model = GTK_TREE_MODEL(task_store);
  task_view = gtk_tree_view_new();
  gtk_tree_view_set_model(GTK_TREE_VIEW(task_view),model);
  g_object_unref(model);

/* main treeview definition */
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(task_view), TRUE);
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title (col, "Tasks");
  gtk_tree_view_append_column (GTK_TREE_VIEW(task_view), col);
  main_renderer[0] = gtk_cell_renderer_pixbuf_new();
  main_renderer[1] = gtk_cell_renderer_text_new();
  main_renderer[2] = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, main_renderer[0], FALSE);
  gtk_tree_view_column_pack_start(col, main_renderer[1], TRUE);
  gtk_tree_view_column_pack_start(col, main_renderer[2], TRUE);
  gtk_tree_view_column_add_attribute(col, main_renderer[0], "pixbuf", ICON_STARTED);
  gtk_tree_view_column_add_attribute(col, main_renderer[1], "text", DESCRIPTION);
  gtk_tree_view_column_add_attribute(col, main_renderer[2], "text", STATUS);
/* the following line is for debugging and index checking */
/*  gtk_tree_view_column_add_attribute(col, main_renderer[2], "text", ID); */
/* remember to comment the row STATUS when unchecking this! */
#ifdef IS_HILDON
  // an aextetical frame only for Hildon
  rframe = gtk_frame_new("Journal");
  gtk_frame_set_label_align (GTK_FRAME(rframe), 0.5, 0.0);
  gtk_frame_set_shadow_type (GTK_FRAME(rframe), GTK_SHADOW_IN);
#endif

/* the selection for the task_view */
  task_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(task_view));
  gtk_tree_selection_set_mode (task_selection, GTK_SELECTION_SINGLE);

/* journal_list is the structure which contains journalling datas */
  journal_list = gtk_list_store_new (JL_NUM_COLS,
    G_TYPE_UINT, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT);
  model = GTK_TREE_MODEL(journal_list);
  journal_view = gtk_tree_view_new_with_model(model);
  g_object_unref(model);

/* journal_list view definition */
  col_journal=gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col_journal,"Journal");
  gtk_tree_view_append_column(GTK_TREE_VIEW(journal_view),col_journal);
  gtk_tree_view_column_set_spacing(col_journal, 3);

  renderer[0]=gtk_cell_renderer_pixbuf_new();
  g_object_set(renderer[0], "yalign", 0.1, NULL);
  gtk_tree_view_column_pack_start(col_journal, renderer[0], FALSE);
  gtk_tree_view_column_add_attribute(col_journal, renderer[0], "pixbuf", JL_ICON);
/*
  renderer[1]=gtk_cell_renderer_pixbuf_new();
  g_object_set(renderer[1], "yalign", 0.1, NULL);
  gtk_tree_view_column_pack_start(col_journal, renderer[1], FALSE);
  gtk_tree_view_column_add_attribute(col_journal, renderer[1], "pixbuf", JL_ICON_STOP);
*/
  renderer[2]=gtk_cell_renderer_text_new();
  g_object_set(renderer[2], "yalign", 0.1, NULL);
  gtk_tree_view_column_pack_start(col_journal, renderer[2], FALSE);
  gtk_tree_view_column_add_attribute(col_journal, renderer[2], "text", JL_INFO);

  renderer[3]=gtk_cell_renderer_text_new();
  g_object_set(renderer[3], "yalign", 0.1, NULL);
  gtk_tree_view_column_pack_start(col_journal, renderer[3], FALSE);
  gtk_tree_view_column_add_attribute(col_journal, renderer[3], "text", JL_DESCRIPTION);
/*
  renderer[4]=gtk_cell_renderer_text_new();
  g_object_set(renderer[4], "yalign", 0.1, NULL);
  gtk_tree_view_column_pack_start(col_journal, renderer[4], TRUE);
  gtk_tree_view_column_add_attribute(col_journal, renderer[4], "text", JL_DURATION);
*/
/* the selection for the journal_view */
  journal_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(journal_view));
  gtk_tree_selection_set_mode (journal_selection, GTK_SELECTION_SINGLE);

#ifndef IS_HILDON
  /* the journal toolbar - this is only for GPE */
  /* for the moment there is only the 'back to task' button */
    journal_toolbar = gtk_toolbar_new ();
    gtk_toolbar_set_orientation (GTK_TOOLBAR (journal_toolbar), GTK_ORIENTATION_HORIZONTAL);
  /* separator */
    sep = gtk_separator_tool_item_new ();
    gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(sep), FALSE);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM(sep), TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR(journal_toolbar), sep, -1);
  /* back to tasks button */
    back = gtk_tool_button_new_from_stock (GTK_STOCK_GO_BACK);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (back),  _("Back to tasks"));
    gtk_toolbar_insert (GTK_TOOLBAR(journal_toolbar), back, -1);
#endif

/* the main toolbar *
** these are objects creation, and it's rather different
** between standard GPE and HILDON */

#ifdef IS_HILDON
  /*new task button*/
    new = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
  /* new sub-task button */
    w = gtk_image_new_from_file(ICON_PATH "/qgn_indi_gene_plus.png");
    new_sub = gtk_tool_button_new(w, _("New subtask"));
  /* delete task button */
    w = gtk_image_new_from_file(ICON_PATH "/qgn_toolb_gene_deletebutton.png");
    delete = gtk_tool_button_new(w, _("Delete task"));
  /* refresh task list button */
    w = gtk_image_new_from_file(ICON_PATH "/qgn_toolb_gene_refresh.png");
    refresh = gtk_tool_button_new(w, _("Refresh tasks list"));
  /* edit task/journal button */
    edit = gtk_tool_button_new_from_stock (GTK_STOCK_EDIT);
    gtk_widget_set_sensitive (GTK_WIDGET(edit), FALSE);
#else
  /* new task button */
    new = gtk_tool_button_new_from_stock (GTK_STOCK_NEW);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (new),  _("New task"));
  /* new sub-task button */
    new_sub = gtk_tool_button_new_from_stock (GTK_STOCK_ADD);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (new_sub),  _("Sub-task"));
  /* delete task button */
    delete = gtk_tool_button_new_from_stock (GTK_STOCK_DELETE);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON(delete), _("Delete task"));
  /* refresh task list button */
    refresh = gtk_tool_button_new_from_stock (GTK_STOCK_REFRESH);
  /* edit task/journal button */
    edit = gtk_tool_button_new_from_stock (GTK_STOCK_EDIT);
  /* show inline journal button */
    p = gpe_find_icon ("journal");
    pw = gtk_image_new_from_pixbuf (p);
    show = gtk_tool_button_new (GTK_WIDGET(pw), _("Show journal"));
#endif

/*this is the code for the sequence of widgets into main toolbar */
  main_toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (main_toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_insert (GTK_TOOLBAR (main_toolbar), new, -1);
  gtk_toolbar_insert (GTK_TOOLBAR (main_toolbar), new_sub, -1);
  gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), delete, -1);
  gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), refresh, -1);
/*the button for editing logs is situated into
** different toolbars between HILDON and GPE*/
  #ifdef IS_HILDON
    gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), edit, -1);
  #else
    gtk_toolbar_insert (GTK_TOOLBAR(journal_toolbar), edit, 0);
  #endif
/* separator */
    sep1 = gtk_separator_tool_item_new();
    gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), sep1, -1);
/* clock-in button */
  p = gpe_find_icon ("clock");
  pw = gtk_image_new_from_pixbuf (p);
  clock_in = gtk_tool_button_new (GTK_WIDGET(pw), _("Clock in"));
  gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), clock_in, -1);
/* stop-clock button */
  p = gpe_find_icon ("stop_clock");
  pw = gtk_image_new_from_pixbuf (p);
  clock_out = gtk_tool_button_new (GTK_WIDGET(pw), _("Clock out"));
  gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), clock_out, -1);
/* separator */
  sep2 = gtk_separator_tool_item_new();
  gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), sep2, -1);

#ifndef IS_HILDON
  /* other stuff only for standard GPE interface
  ** adding help button if help exists */
    gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), show, -1);
    if(gpe_check_for_help(appname) != NULL)
      {
        GtkToolItem *help_icon;
        pw = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_SMALL_TOOLBAR);
        help_icon = gtk_tool_button_new (GTK_WIDGET(pw), _("Help"));
        gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), help_icon, -1);
        g_signal_connect (G_OBJECT(help_icon), "clicked",
                          G_CALLBACK (show_help), NULL);
      }
  /* adding separation */
    sep3 = gtk_separator_tool_item_new ();
    gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM(sep3), FALSE);
    gtk_tool_item_set_expand (GTK_TOOL_ITEM(sep3), TRUE);
    gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), sep3, -1);
  /* adding exit app button */
    quit = gtk_tool_button_new_from_stock (GTK_STOCK_QUIT);
    gtk_toolbar_insert (GTK_TOOLBAR(main_toolbar), quit, -1);
#endif

#ifdef IS_HILDON
/* this is Hildon Main Menu */

  /*main menu*/
    GtkMenu   *menu_main = hildon_appview_get_menu(HILDON_APPVIEW(main_window));
  /*view menu declarations*/
    GtkWidget *menu_view = gtk_menu_new();
    GtkWidget *item_view = gtk_menu_item_new_with_label(_("View"));
    GSList    *menu_view_group = NULL;
    GtkWidget *item_view_both = gtk_radio_menu_item_new_with_label(menu_view_group, _("Timesheet & Todo items"));
    menu_view_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item_view_both));
    GtkWidget *item_view_task = gtk_radio_menu_item_new_with_label(menu_view_group, _("Only timesheet tasks"));
    menu_view_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item_view_task));
    GtkWidget *item_view_todo = gtk_radio_menu_item_new_with_label(menu_view_group, _("Only todo items"));
    menu_view_group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item_view_todo));
  /*show menu declarations*/
    GtkWidget *menu_show = gtk_menu_new();
    GtkWidget *item_show = gtk_menu_item_new_with_label(_("Show"));
    GtkWidget *item_switch_toolbar = gtk_check_menu_item_new_with_label(_("Show toolbar"));
    GtkWidget *item_switch_journal = gtk_check_menu_item_new_with_label(_("Show journal"));
  /*menu building*/
    if (!todo_db_start())
      {/* only if we found the todo db*/
        gtk_menu_append (GTK_MENU(menu_main), item_view);
        gtk_menu_append (GTK_MENU(menu_view), item_view_both);
        gtk_menu_append (GTK_MENU(menu_view), item_view_task);
        gtk_menu_append (GTK_MENU(menu_view), item_view_todo);
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(item_view), menu_view);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item_view_both), TRUE);
      }
  
    gtk_menu_append (GTK_MENU(menu_main), item_show);
    gtk_menu_append (GTK_MENU(menu_show), item_switch_toolbar);
    gtk_menu_append (GTK_MENU(menu_show), item_switch_journal);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM(item_show), menu_show);
  /*menu initial settings*/
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_switch_toolbar), TRUE);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item_switch_journal), FALSE);
  /* fire! */
    gtk_widget_show_all (GTK_WIDGET(menu_main));
  /*these are necessary to show all toolbar's items*/
    hildon_appview_set_toolbar(HILDON_APPVIEW(main_window), GTK_TOOLBAR(main_toolbar));
    gtk_widget_show_all (GTK_WIDGET(main_window));
#endif

/* here starts the real stuff
 * this is the definition of the layout on screen! */

  main_vbox = gtk_vbox_new (FALSE,0);
  main_scrolled_window = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(main_scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  journal_scrolled_window = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(journal_scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

#ifdef IS_HILDON
/* let's configure to have journal permanently on-screen */
  main_hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), main_hpaned, TRUE, TRUE, 0);
  gtk_paned_pack1 (GTK_PANED (main_hpaned), main_scrolled_window, TRUE, FALSE);
  gtk_container_add (GTK_CONTAINER (main_scrolled_window), task_view);
  gtk_widget_set_size_request (task_view, 160, -1);
  gtk_paned_pack2 (GTK_PANED (main_hpaned), rframe, TRUE, TRUE);
  gtk_paned_set_position (GTK_PANED(main_hpaned), 120);
  gtk_container_add (GTK_CONTAINER(rframe), journal_scrolled_window);
  gtk_container_add (GTK_CONTAINER(journal_scrolled_window), journal_view);
#else
/*  let's configure the main vbox containing
    - main toolbar (already added some lines ago...)
    - journal toolbar (already added some lines ago...)
    - two different boxes in order to exchange them when switching from tasks to journal
*/
  journal_vbox = gtk_vbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), journal_toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), main_scrolled_window, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (main_scrolled_window), task_view);
  gtk_box_pack_start (GTK_BOX (main_vbox), journal_vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (journal_vbox), journal_scrolled_window, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER(journal_scrolled_window), journal_view);
#endif


/* here are grouped signal connections ! */

  /* change line */
    /* when we change line we have to;
      1 - check the status of clock_in and clock_out button
      2 - update the journal if we are in Hildon
    */
  g_signal_connect (G_OBJECT(task_selection), "changed", G_CALLBACK(view_selected_row_cb), NULL);

  /* here are common signals for the main_toolbar */
  g_signal_connect (G_OBJECT (new), "clicked",
                        G_CALLBACK (ui_new_task), task_store);
  g_signal_connect (G_OBJECT (new_sub), "clicked",
                        G_CALLBACK (ui_new_sub_task), task_selection);
  g_signal_connect (G_OBJECT(delete), "clicked",
                    G_CALLBACK (ui_delete_task), task_selection);
  g_signal_connect (G_OBJECT(clock_in), "clicked",
                    G_CALLBACK (start_timing), task_selection);
  g_signal_connect (G_OBJECT(clock_out), "clicked",
                    G_CALLBACK (stop_timing), task_selection);
  /* signal for the edit journal button
  ** remember that it is on two different toolbars in the interfaces
  ** also if it has same name and task and thus common signal */
  g_signal_connect (G_OBJECT(edit), "clicked",
                    G_CALLBACK (ui_edit_journal), journal_selection);
  /* we have to check every time we change row in the journal
  ** in order to determine button status and other interface tasks */
    g_signal_connect (G_OBJECT(journal_selection), "changed",
                    G_CALLBACK(journal_changed_row_cb), edit);

  #ifdef IS_HILDON
    /* the signal to update the onscreen journal every time we change row */
      g_signal_connect (G_OBJECT(task_selection), "changed",
                        G_CALLBACK (prepare_onscreen_journal), NULL);
    /*here follows signals for the menu/toolbar voices*/
    /* this is the signal to activate and deactivate hildon toolbar */
      g_signal_connect(G_OBJECT(item_switch_toolbar), "activate",
        G_CALLBACK(toggle_toolbar), main_toolbar);
    /* this is the one to show or hide the journal */
      g_signal_connect(G_OBJECT(item_switch_journal), "activate",
        G_CALLBACK(toggle_journal), rframe);
    /*these are filtering view options for tasks and todos
    ** only if we found the todo db */
      if (!todo_db_start())
        {
          g_signal_connect(G_OBJECT(item_view_both), "toggled",
            G_CALLBACK(filter_show_both), task_view);
          g_signal_connect(G_OBJECT(item_view_task), "toggled",
            G_CALLBACK(filter_show_task), task_view);
          g_signal_connect(G_OBJECT(item_view_todo), "toggled",
          G_CALLBACK(filter_show_todo), task_view);
        }
    /* we also have the menu for change visuals between todos and tasks*/
      g_signal_connect (G_OBJECT(refresh), "clicked",
                      G_CALLBACK (refresh_list), item_view_both);
  #else
    /* signal for refreshing task list */
    g_signal_connect (G_OBJECT(refresh), "clicked",
                    G_CALLBACK (refresh_list), NULL);
    /*  These are signals for the show journal button
        when clicked this button should:
          1 - prepare the journal
          2 - hide main_scrolled_window
          3 - show journal_vbox
          4 - hide main_toolbar
          5 - show journal_toolbar */
    g_signal_connect_swapped (G_OBJECT(show), "clicked",
                    G_CALLBACK(prepare_onscreen_journal), G_OBJECT(task_selection));
    g_signal_connect_swapped (G_OBJECT(show), "clicked",
                    G_CALLBACK(gtk_widget_hide), G_OBJECT(main_scrolled_window));
    g_signal_connect_swapped (G_OBJECT(show), "clicked",
                    G_CALLBACK(gtk_widget_show), G_OBJECT(journal_vbox));
    g_signal_connect_swapped (G_OBJECT(show), "clicked",
                    G_CALLBACK(gtk_widget_hide), G_OBJECT(main_toolbar));
    g_signal_connect_swapped (G_OBJECT(show), "clicked",
                    G_CALLBACK(gtk_widget_show), G_OBJECT(journal_toolbar));
  /* we create also the signal for the quit buttons */
    g_signal_connect (G_OBJECT(quit), "clicked",
                      G_CALLBACK (gtk_main_quit), NULL);
  /* these are signals for the journal toolbar */
    /*  if we press the 'back' button one we have to:
        1 - hide the journal
        2 - hide the journal_toolbar
        3 - show the main toolbar
        4 - show the tasks_view */
    g_signal_connect_swapped (G_OBJECT(back), "clicked",
                      G_CALLBACK (gtk_widget_hide), journal_vbox);
    g_signal_connect_swapped (G_OBJECT(back), "clicked",
                              G_CALLBACK (gtk_widget_hide), journal_toolbar);
    g_signal_connect_swapped (G_OBJECT(back), "clicked",
                      G_CALLBACK (gtk_widget_show), main_toolbar);
    g_signal_connect_swapped (G_OBJECT(back), "clicked",
                      G_CALLBACK (gtk_widget_show), main_scrolled_window);

  #endif

/* last stuff */
btn_con = GTK_WIDGET(clock_in);
btn_coff = GTK_WIDGET(clock_out);
set_active (0,0);
gtk_widget_show_all (main_vbox);

#ifndef IS_HILDON
/* if we are not in Hildon we should hide the journal specific toolbar */
gtk_widget_hide(journal_vbox);
gtk_widget_hide(journal_toolbar);
#else
/* otherwise we should hide the right frame containing the journal */
gtk_widget_hide(rframe);
#endif

return main_vbox;

}
