/*
 * This file is part of gpe-timeheet
 * (c) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 * (c) 2005 Philippe De Swert <philippedeswert@scarlet.be>
 * (c) 2006 Michele Giorgini <md6604@mclink.it>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include "journal.h"

#define _(x) gettext(x)

static time_t totaltime = 0;

GtkListStore  *journal_list;
GtkTreeIter   iter;

/* adds the document header */
void
journal_list_header(char* title)
{
  gtk_list_store_clear (journal_list);
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set (journal_list, &iter,
    JL_INFO, _("Date/Duration"),
    JL_DESCRIPTION, _("Start/End Notes"),
    -1);
  totaltime = 0;
  return;
}

/* adding data lines to journal listview */
void
journal_list_add_line(time_t tstart, time_t tstop, 
                 const char *istart, const char *istop, guint *idx, guint status)
{
  GdkPixbuf *icon_start, *icon_stop;
  char *starttm, *stoptm, *tm;
  char duration[24];

  tm = malloc(sizeof(char)*26);

  tm = ctime_r(&tstart, tm);
  starttm = g_strndup(tm, 24);

  tm = ctime_r(&tstop, tm);
  stoptm = g_strndup (tm, 24);

  icon_start = gpe_find_icon("media_play");
  icon_stop = gpe_find_icon("media_stop");

  sprintf(duration, "%.2ld:%.2ld:%.2ld\n", (tstop - tstart)/3600, ((tstop - tstart)%3600)/60, (((tstop - tstart)%3600)%60));
  
  /* in order to have a better appearance on small screen
  ** expecially in narrow ones, we'll use two rows for every journal line
  ** one for the start log and the other for the end log */
  if (status)
    {
      gtk_list_store_append(journal_list, &iter);
      gtk_list_store_set(journal_list, &iter,
        JL_ID, *idx,
        JL_ICON, icon_start,
        JL_INFO, starttm,
        JL_DESCRIPTION, istart,
        JL_START, tstart,
        JL_STOP, tstop,
        JL_START_NOTES, istart,
        JL_STOP_NOTES, istop,
        JL_TYPE, JL_TYPE_START,
        -1);

      gtk_list_store_append(journal_list,&iter);
      gtk_list_store_set(journal_list, &iter,
        JL_ID, *idx,
        JL_ICON, icon_stop,
        JL_INFO, duration,
        JL_DESCRIPTION, istop,
        JL_START, tstart,
        JL_STOP, tstop,
        JL_START_NOTES, istart,
        JL_STOP_NOTES, istop,
        JL_TYPE, JL_TYPE_STOP,
        -1);
    }
  else
    {
      gtk_list_store_append(journal_list, &iter);
      gtk_list_store_set(journal_list, &iter,
        JL_ID, *idx,
        JL_ICON, icon_start,
        JL_INFO, starttm,
        JL_DESCRIPTION, istart,
        JL_START, tstart,
        JL_STOP, tstop,
        JL_START_NOTES, istart,
        JL_STOP_NOTES, istop,
        JL_TYPE, JL_TYPE_START_OPEN,
        -1);

      gtk_list_store_append(journal_list,&iter);
      gtk_list_store_set(journal_list, &iter,
        JL_ID, *idx,
        JL_ICON, icon_stop,
        JL_INFO, duration,
        JL_DESCRIPTION, istop,
        JL_START, tstart,
        JL_STOP, tstop,
        JL_START_NOTES, istart,
        JL_STOP_NOTES, istop,
        JL_TYPE, JL_TYPE_STOP_OPEN,
        -1);
      }

  totaltime = totaltime + (tstop - tstart);
  g_free(starttm);
  g_free(stoptm);
  g_free(tm);
}

/* add footer to the journal list */
void
journal_list_footer()
{
  char duration[25];
  
  sprintf(duration, "%.2ld g. %.2ld h. %.2ld m. %.2ld s.\0", ((totaltime)/86400), ((totaltime)%86400)/3600, (((totaltime)%86400)%3600)/60, (((totaltime)%3600)%60));
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set (journal_list, &iter, JL_INFO, _("Total time"), -1);
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set (journal_list, &iter, 
                      JL_INFO, duration,
                      JL_STOP, totaltime,
                      JL_TYPE, JL_TYPE_LAST,
                      -1);

}

static void
confirm_click_ok (GtkWidget *widget, gpointer p)
{
  GtkWindow *dw = p;

  GtkWidget *start_date = gtk_object_get_data(GTK_OBJECT(dw),"startd");
  GtkWidget *start_hh = gtk_object_get_data(GTK_OBJECT(dw),"starth");
  GtkWidget *start_mm = gtk_object_get_data(GTK_OBJECT(dw),"startm");
  GtkWidget *start_ss = gtk_object_get_data(GTK_OBJECT(dw),"starts");
  GtkWidget *stop_date = gtk_object_get_data(GTK_OBJECT(dw),"stopd");
  GtkWidget *stop_hh = gtk_object_get_data(GTK_OBJECT(dw),"stoph");
  GtkWidget *stop_mm = gtk_object_get_data(GTK_OBJECT(dw),"stopm");
  GtkWidget *stop_ss = gtk_object_get_data(GTK_OBJECT(dw),"stops");

  struct tm curr_time;
  time_t tstart, tstop;
  time_t *next_start, *previous_stop;

  previous_stop = gtk_object_get_data(GTK_OBJECT(dw),"prevstop");
  next_start = gtk_object_get_data(GTK_OBJECT(dw),"nextstart");

  /* prepare start log datas */
    curr_time.tm_year = GTK_DATE_COMBO(start_date)->year - 1900;
    curr_time.tm_mon = GTK_DATE_COMBO(start_date)->month;
    curr_time.tm_mday = GTK_DATE_COMBO(start_date)->day;
    curr_time.tm_hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_hh));
    curr_time.tm_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_mm));
    curr_time.tm_sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_ss));
    tstart = mktime (&curr_time);

  /* prepare stop log datas*/
    curr_time.tm_year = GTK_DATE_COMBO(stop_date)->year - 1900;
    curr_time.tm_mon = GTK_DATE_COMBO(stop_date)->month;
    curr_time.tm_mday = GTK_DATE_COMBO(stop_date)->day;
    curr_time.tm_hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_hh));
    curr_time.tm_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_mm));
    curr_time.tm_sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_ss));
    tstop = mktime (&curr_time);

  /* now let's check! */
      if ((tstart < *previous_stop || tstop > *next_start) && (tstop > tstart))
        {
          gpe_error_box(_("Please check data inserted! Time data out of range for the current log or stop before start"));
        }
      else
        gtk_main_quit ();
}

static void
confirm_click_cancel (GtkWidget *widget, gpointer p)
{
  gtk_widget_destroy (GTK_WIDGET (p));
}

static void
confirm_edit_destruction (GtkWidget *widget, gpointer p)
{ /* this ends modifying logs without changes */
  gboolean *b = (gboolean *)p;

  if (*b == FALSE)
    {
      *b = TRUE;

      gtk_main_quit ();
    }
}

static toggle_in_progress (GtkWidget *check, gpointer user_data)
{
  GtkWidget *w = user_data;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check)))
      gtk_widget_set_sensitive (w , FALSE);
  else
      gtk_widget_set_sensitive (w, TRUE);
}

static void
load_journal_children(GtkTreeModel *model, GtkTreeIter iter)
{
  GtkTreeIter child;
  
  if (gtk_tree_model_iter_has_child (model, &iter))
    {
      gtk_tree_model_iter_children(model, &child, &iter);
      load_journal_children (model, child);
    }
  else
    {
      gint id;

      gtk_tree_model_get (model, &iter, ID, &id, -1);
      scan_journal (id);

      if (gtk_tree_model_iter_next(model, &iter))
        {
          load_journal_children (model, iter);
        }
      else
        {
          return;
        }
    }
}

void
prepare_onscreen_journal (GtkTreeSelection *selection, gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;
  gint id;
  gchar *description;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, ID, &id, DESCRIPTION, &description, -1);
      /* first of all we add the header */
      journal_list_header(description);
      /* current item should be read prior to recurse the tree */
      gtk_tree_model_get (model, &iter, ID, &id, DESCRIPTION, &description, -1);
      scan_journal (id);
      /* if the current node has children, start reading recursively the tree */
      if (gtk_tree_model_iter_has_child (model, &iter))
        load_journal_children(model, iter);
      /* and at last add the footer */
      journal_list_footer();
    }
  else
    {
      #ifndef IS_HILDON
      /* this error message should appear only in standard GPE
      ** because HILDON interface has on-screen journal and it
      ** simply stays empty */
      gpe_error_box(_("No data selected for journal!"));
      #endif
    }
}

gboolean
ui_edit_journal (GtkWidget *w, gpointer user_data)
{ /* edit a journal voice selected from the on-screen journal */

  GtkWidget *dw = gtk_dialog_new ();
  gboolean destroyed = FALSE;
  GtkWidget *buttonok, *buttoncancel;
  GdkPixbuf *pixbuf;
  GtkWidget *icon;
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *start_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *stop_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *progress_hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *vbox2 = gtk_vbox_new (FALSE, 0);
  GtkWidget *edit_label, *taskname_label, *warning_label;
  GtkWidget *start_frame, *start_entry, *start_note, *start_date, *start_hh, *start_mm, *start_ss;
  GtkWidget *stop_frame, *stop_entry, *stop_note, *stop_date, *stop_hh, *stop_mm, *stop_ss;
  //GtkWidget *check_in_progress;

  GtkTreeSelection  *selection = user_data;
  GtkTreeIter iter, current_start;
  GtkTreeModel *model;
  GtkTreePath *path;

  int idx, i, type, test;
  time_t tstart, tstop, previous_stop, next_start;
  time_t old_tstart, old_tstop, old_logtime, old_totaltime;
  const gchar *text_start, *text_stop;
  gchar *start_time = g_malloc(sizeof(char)*26);
  gchar *stop_time = g_malloc(sizeof(char)*26);
  gchar *start_notes = g_malloc (sizeof(char)*80);
  gchar *stop_notes = g_malloc (sizeof(char)*80);
  char duration[25];

  struct tm curr_time;
  struct task *t;

  t = g_malloc (sizeof (struct task));

  gtk_window_set_title (GTK_WINDOW (dw), _("TimeTracker: edit log"));

/*TODO: we need to improve graphic presentation of this window!*/

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (vbox2);

  if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
      /*edit_label = gtk_label_new (_("Editing log"));
        gtk_box_pack_start (GTK_BOX (vbox), edit_label, FALSE, FALSE, 0);
        gtk_widget_show (edit_label);
        gtk_misc_set_alignment (GTK_MISC (edit_label), 0.0, 0.5);

      /* retrieve start and end time of current log */
      gtk_tree_model_get (model, &iter, 
            JL_ID, &idx, 
            JL_START, &tstart,
            JL_STOP, &tstop,
            JL_START_NOTES, &start_notes,
            JL_STOP_NOTES, &stop_notes,
            JL_TYPE, &type,
            -1);

      start_time = ctime_r(&tstart, start_time);
      stop_time = ctime_r(&tstop, stop_time);
      old_logtime = tstop-tstart;
      old_tstart = tstart;
      old_tstop = tstop;

      path = gtk_tree_model_get_path(model, &iter);
      gtk_tree_path_prev(path);

      if ((type==JL_TYPE_STOP) || (type==JL_TYPE_STOP_OPEN))
        { /* stop case -> we need to go back two positions
          ** in the list before start fetching */
          gtk_tree_path_prev(path);
        }

      if ((gtk_tree_model_get_iter(model, &iter, path)))
        {
          gtk_tree_model_get (model, &iter, JL_STOP, &previous_stop, -1);
          gtk_tree_model_iter_next(model, &iter);
          /* we are now pointing to the START log item
          ** and we remember the current iter for future use */
          current_start = iter;
          gtk_tree_model_iter_next(model, &iter);
          if (gtk_tree_model_iter_next(model, &iter))
            gtk_tree_model_get (model, &iter, JL_START, &next_start, -1);
          else
            /* we should never arrive here ! If it happens, there's something
            ** strange with DB */
            gpe_error_box(_("It seems that something is corrupted... Pls restart the timesheet application!"));
            //next_start = 0;
          if (!next_start)
            /* we are on the last log or the item is open!
            let's fix next_start = current time */
            next_start = time(&next_start);
        }
      else
        { /* again, we should never arrive here! if it happens, there's
          ** something wrong with the archives */
          gpe_error_box(_("It seems that something is corrupted... Pls restart the timesheet application!"));
        }

      /* it could be nice also to retrieve the name of the task we are currently editing 
      text = read_task_name(idx);
      taskname_label = gtk_label_new(text);
      gtk_box_pack_start (GTK_BOX (vbox), taskname_label, FALSE, FALSE, 0);
      gtk_widget_show (taskname_label);
      gtk_misc_set_alignment (GTK_MISC (taskname_label), 0.0, 0.5);*/

      gtk_box_pack_start (GTK_BOX (vbox2), vbox, TRUE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 8);

      pixbuf = gpe_find_icon ("gpe-timesheet");
      if (pixbuf)
        {
          icon = gtk_image_new_from_pixbuf (pixbuf);
          gtk_box_pack_end (GTK_BOX (hbox), icon, FALSE, FALSE, 0);
          gtk_widget_show (icon);
        }

      /*check_in_progress = gtk_check_button_new_with_label (_("Task in progress"));
      gtk_box_pack_start (GTK_BOX (hbox), check_in_progress, FALSE, FALSE, 0);*/
      warning_label = gtk_label_new(_("Warning: this is an open log! Pressing OK means the confirmation of a STOP log event!"));
      gtk_label_set_line_wrap (GTK_LABEL(warning_label), TRUE);
      gtk_box_pack_start (GTK_BOX (hbox), warning_label, FALSE, FALSE, 0);
      start_frame = gtk_frame_new (_("Start datas"));
      stop_frame = gtk_frame_new (_("Stop datas"));
      gtk_widget_show (start_frame);
      gtk_widget_show (stop_frame);

      /* preparing start log datas */
      localtime_r (&tstart, &curr_time);
      start_date = gtk_date_combo_new();
      gtk_date_combo_set_date(GTK_DATE_COMBO(start_date),
            curr_time.tm_year+1900,
            curr_time.tm_mon,
            curr_time.tm_mday);

      start_hh = gtk_spin_button_new_with_range(0,23,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(start_hh), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(start_hh), curr_time.tm_hour);
      start_mm = gtk_spin_button_new_with_range(0,59,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(start_mm), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(start_mm), curr_time.tm_min);
      start_ss = gtk_spin_button_new_with_range(0,59,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(start_ss), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(start_ss), curr_time.tm_sec);

      start_note = gtk_entry_new ();
      gtk_entry_set_text(GTK_ENTRY(start_note),start_notes);

      /*preparing stop log datas */
      localtime_r (&tstop, &curr_time);
      stop_date = gtk_date_combo_new();
      gtk_date_combo_set_date(GTK_DATE_COMBO(stop_date),
            curr_time.tm_year+1900,
            curr_time.tm_mon,
            curr_time.tm_mday);

      stop_hh = gtk_spin_button_new_with_range(0,23,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(stop_hh), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(stop_hh), curr_time.tm_hour);
      stop_mm = gtk_spin_button_new_with_range(0,59,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(stop_mm), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(stop_mm), curr_time.tm_min);
      stop_ss = gtk_spin_button_new_with_range(0,59,1);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON(stop_ss), TRUE);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON(stop_ss), curr_time.tm_sec);

      stop_note = gtk_entry_new ();
      gtk_entry_set_text(GTK_ENTRY(stop_note),stop_notes);

      /* packing start log box */
      gtk_box_pack_start (GTK_BOX(start_hbox), start_date, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(start_hbox), start_hh, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(start_hbox), start_mm, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(start_hbox), start_ss, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(start_hbox), start_note, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER(start_frame),start_hbox);

      /* packing stop log box */
      gtk_box_pack_start (GTK_BOX(stop_hbox), stop_date, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(stop_hbox), stop_hh, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(stop_hbox), stop_mm, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(stop_hbox), stop_ss, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX(stop_hbox), stop_note, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER(stop_frame), stop_hbox);

      /* show start & stop log sections */
      gtk_widget_show (start_date);
      gtk_widget_show (start_hh);
      gtk_widget_show (start_mm);
      gtk_widget_show (start_ss);
      gtk_widget_show (stop_date);
      gtk_widget_show (stop_hh);
      gtk_widget_show (stop_mm);
      gtk_widget_show (stop_ss);

      /* packing the whole window */
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->vbox), hbox, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->vbox), start_frame, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->vbox), stop_frame, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->vbox), progress_hbox, FALSE, FALSE, 0);

      /* settings some data that will be transmitted to callback clicked_ok*/
      gtk_object_set_data(GTK_OBJECT(hbox),"stopd", GTK_DATE_COMBO(stop_date));
      gtk_object_set_data(GTK_OBJECT(hbox),"stoph", stop_hh);
      gtk_object_set_data(GTK_OBJECT(hbox),"stopm", stop_mm);
      gtk_object_set_data(GTK_OBJECT(hbox),"stops", stop_ss);
      gtk_object_set_data(GTK_OBJECT(hbox),"startd", GTK_DATE_COMBO(start_date));
      gtk_object_set_data(GTK_OBJECT(hbox),"starth", start_hh);
      gtk_object_set_data(GTK_OBJECT(hbox),"startm", start_mm);
      gtk_object_set_data(GTK_OBJECT(hbox),"starts", start_ss);
      gtk_object_set_data(GTK_OBJECT(hbox),"prevstop", &previous_stop);
      gtk_object_set_data(GTK_OBJECT(hbox),"nextstart", &next_start);

      buttonok = gtk_button_new_from_stock (GTK_STOCK_OK);
      buttoncancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->action_area), 
		      buttoncancel, TRUE, TRUE, 0);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dw)->action_area), 
		      buttonok, TRUE, TRUE, 0);

      /* connecting signals to objects */
      g_signal_connect (G_OBJECT (buttonok), "clicked", 
		      G_CALLBACK (confirm_click_ok), GTK_OBJECT(hbox));
      g_signal_connect (G_OBJECT (buttoncancel), "clicked", 
		      G_CALLBACK (confirm_click_cancel), dw);
      /* when toggling the checkbox, we have to disable 5 controls! 
      g_signal_connect (G_OBJECT (check_in_progress), "toggled",
              G_CALLBACK (toggle_in_progress), stop_hh);
      g_signal_connect (G_OBJECT (check_in_progress), "toggled",
              G_CALLBACK (toggle_in_progress), stop_mm);
      g_signal_connect (G_OBJECT (check_in_progress), "toggled",
              G_CALLBACK (toggle_in_progress), stop_ss);
      g_signal_connect (G_OBJECT (check_in_progress), "toggled",
              G_CALLBACK (toggle_in_progress), stop_note);
      g_signal_connect (G_OBJECT (check_in_progress), "toggled",
              G_CALLBACK (toggle_in_progress), stop_date); */

      g_signal_connect (G_OBJECT (dw), "destroy",
              G_CALLBACK(confirm_edit_destruction), &destroyed);

      gtk_window_set_modal (GTK_WINDOW (dw), TRUE);
      gtk_widget_show_all (dw);
      if (!((type == JL_TYPE_START_OPEN) || (type == JL_TYPE_STOP_OPEN)))
        gtk_widget_hide(warning_label);
      gtk_dialog_set_has_separator (GTK_DIALOG (dw), FALSE);
      gtk_widget_grab_focus (GTK_WIDGET (start_note));

      gtk_main ();

      if (destroyed)
        return FALSE;

      /* if we arrive here we have to save editing of logs*/ 

      /* prepare start log datas */
      curr_time.tm_year = GTK_DATE_COMBO(start_date)->year - 1900;
      curr_time.tm_mon = GTK_DATE_COMBO(start_date)->month;
      curr_time.tm_mday = GTK_DATE_COMBO(start_date)->day;
      curr_time.tm_hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_hh));
      curr_time.tm_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_mm));
      curr_time.tm_sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON(start_ss));
      tstart = mktime (&curr_time);
      text_start = gtk_entry_get_text(GTK_ENTRY(start_note));
      /* prepare stop log datas*/
      curr_time.tm_year = GTK_DATE_COMBO(stop_date)->year - 1900;
      curr_time.tm_mon = GTK_DATE_COMBO(stop_date)->month;
      curr_time.tm_mday = GTK_DATE_COMBO(stop_date)->day;
      curr_time.tm_hour = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_hh));
      curr_time.tm_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_mm));
      curr_time.tm_sec = gtk_spin_button_get_value (GTK_SPIN_BUTTON(stop_ss));
      tstop = mktime (&curr_time);
      text_stop = gtk_entry_get_text(GTK_ENTRY(stop_note));
      /* prepare start_time string for visualization*/
      start_time = ctime_r(&tstart, start_time);
      *(start_time+24) = 0;

      /* when we change log datas, we'll verify that the new data & time
      ** for the log resides in an accettable range, i.e. start after previous log
      ** and finish before next log */

      if ((tstart<previous_stop || tstop>next_start) && (tstop>tstart))
        {
          gpe_error_box(_("Please check data inserted! Time data out of range for the current log or stop before start"));
        }
      else
        { /* let's update logs!*/
/*TODO: check if update_log had success before updating the onscreen_juornal*/
          update_log(text_start, old_tstart, tstart, idx, START);
          t->id = idx;
          if ((type == JL_TYPE_START_OPEN) || (type == JL_TYPE_STOP_OPEN))
            log_entry(STOP, tstop, t, text_stop);
          else
            update_log(text_stop, old_tstop, tstop, idx, STOP);
        /* and update the onscreen journal:
        ** we have to update the two modified logs 
        ** and also the total time! */
          gtk_list_store_set (journal_list, &current_start,
                  JL_INFO, start_time,
                  JL_DESCRIPTION, text_start,
                  JL_START, tstart,
                  JL_STOP, tstop,
                  JL_START_NOTES, text_start,
                  JL_STOP_NOTES, text_stop,
                  JL_TYPE, JL_TYPE_START,
                  -1);
          gtk_tree_model_iter_next(model, &current_start);
          sprintf(duration, "%.2ld:%.2ld:%.2ld\n", (tstop - tstart)/3600, ((tstop - tstart)%3600)/60, (((tstop - tstart)%3600)%60));
          gtk_list_store_set (journal_list, &current_start,
                  JL_INFO, duration,
                  JL_DESCRIPTION, text_stop,
                  JL_START, tstart,
                  JL_STOP, tstop,
                  JL_START_NOTES, text_start,
                  JL_STOP_NOTES, text_stop,
                  JL_TYPE, JL_TYPE_STOP,
                  -1);
          /* retrieve the old totaltime */
          while (gtk_tree_model_iter_next(model, &current_start))
            {
              gtk_tree_model_get(model, &current_start,
                                JL_TYPE, &test,
                                -1);
              if (test == JL_TYPE_LAST)
                {
                  totaltime = totaltime - old_logtime + (tstop-tstart);
                  sprintf(duration, "%.2ld h. %.2ld m. %.2ld s.\0", (totaltime)/3600, ((totaltime)%3600)/60, (((totaltime)%3600)%60));
                  gtk_list_store_set (journal_list, &current_start,
                    JL_INFO, duration,
                    JL_START, totaltime,
                    JL_STOP, totaltime,
                    -1);
                }
            }
          destroyed = TRUE;
          gtk_widget_destroy (dw);
          g_free(t);
          return TRUE;
        }
    }
  else
    { /* we should never be here */
      g_print ("no log selected to edit!\n");
    }
}
