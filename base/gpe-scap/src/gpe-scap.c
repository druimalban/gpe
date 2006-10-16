/* GPE SCAP
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
 * Copyright (C) 2006  Florian Boor <florian@linuxtogo.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include "glade-utils.h"
#include "scr-i18n.h"
#include "scr-shot.h"
#include "cfgfile.h"

#ifdef G_THREADS_IMPL_NONE
#error no threads support
#endif /* G_THREADS_IMPL_NONE */

#define SCR_SCALING_FACTOR 0.4
#define MAX_SCREENSHOTS 1000

enum
{
  DEST_FILE,
  DEST_UPLOAD,
  NUM_DESTS
};

enum
{
  PATH_VIEW_COLUMN,
  PATH_SYSTEM_COLUMN,
  NUM_PATH_COLUMNS
};

typedef struct
{
  GtkDialog *dlg;

  GtkDrawingArea *preview_area;
  GtkRadioButton *dest[NUM_DESTS];

  GtkComboBoxEntry *entry;
  GtkButton *browse_button;

  GtkButton *ok_button;
  GtkButton *cancel_button;

  ScrShot *shot;
  GdkPixbuf *preview;
  GdkPixbuf *preview_scaled;

  GtkDialog *upload_dlg;

  GtkDialog *upload_progress_dlg;
  GtkProgressBar *upload_progress_bar;

  GError *upload_error;
  gchar *upload_response;
  guint pulse_sid;

  GError *save_error;
  gchar *save_path;

  GtkToggleButton *disable_warning_cb;
  t_gpe_scap_cfg *cfg;
  
} ScrMainDialog;

gint
scr_delete_event (GtkWidget * widget, GdkEvent event, gpointer data)
{
  /* just close the window */
  return FALSE;
}

void
scr_quit (GtkWidget *widget, gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  if (mainDlg)
    scr_shot_free (mainDlg->shot);

  gtk_main_quit ();
}

gboolean
scr_upload_pulse (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  if (mainDlg->upload_progress_bar) /* indicates that the dialog was destroyed */
      gtk_progress_bar_pulse (mainDlg->upload_progress_bar);
  else 
      return FALSE;

  /* run forever */
  return TRUE;
}

gboolean
scr_upload_done (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (mainDlg->dlg),
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_CLOSE,
                                              "%s",
                                              mainDlg->upload_response);
  /* using a pulsing progress bar for now */
  /* gtk_progress_bar_set_fraction (mainDlg->upload_progress_bar, (gdouble) 1.0); */
  g_source_remove (mainDlg->pulse_sid);
  gtk_widget_hide (GTK_WIDGET (mainDlg->upload_progress_dlg));

  /* no need to read the response */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));

  /* free memory */
  g_free (mainDlg->upload_response);
  mainDlg->upload_response = NULL;
  
  scr_quit (NULL, data);

  /* remove source */
  return FALSE;
}

gboolean
scr_upload_failed (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  GtkWidget *dialog;
  
  g_source_remove (mainDlg->pulse_sid);
  gtk_widget_hide (GTK_WIDGET (mainDlg->upload_progress_dlg));

  if (mainDlg->upload_error && mainDlg->upload_error->message)
    dialog = gtk_message_dialog_new (GTK_WINDOW (mainDlg->dlg),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error uploading screenshot: %s"),
                                     mainDlg->upload_error->message);
  else
    dialog = gtk_message_dialog_new (GTK_WINDOW (mainDlg->dlg),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Unknown error uploading screenshot."));
  
  
  /* no need to read the response */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));

  /* restore main dialog sensitivity and cursor */
  gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->dlg), TRUE);
  gdk_window_set_cursor (GTK_WIDGET (mainDlg->dlg)->window, NULL);

  /* free memory */
  if (mainDlg->upload_error)
    {
      g_error_free (mainDlg->upload_error);
      mainDlg->upload_error = NULL;
    }

  /* remove source */
  return FALSE;
}

gpointer
scr_async_upload (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  GError *error = NULL;
  gboolean upload_successful;

  upload_successful = scr_shot_upload (mainDlg->shot, "www.handhelds.org/scap/capture.cgi", &mainDlg->upload_response, &mainDlg->upload_error);
  
  if (upload_successful)
    g_idle_add (scr_upload_done, data);
  else
    g_idle_add (scr_upload_failed, data);

  return NULL;
}

gboolean
scr_save_done (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  g_free (mainDlg->save_path);

  scr_quit (NULL, data);

  return FALSE;
}

gboolean
scr_save_failed (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  GtkWidget *dialog;

  if (mainDlg->save_error && mainDlg->save_error->message)
    dialog = gtk_message_dialog_new (GTK_WINDOW (mainDlg->dlg),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Error saving screenshot: %s"),
                                     mainDlg->save_error->message);
  else
    dialog = gtk_message_dialog_new (GTK_WINDOW (mainDlg->dlg),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Unknown error saving screenshot."));
  
  /* no need to read the response */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (GTK_WIDGET (dialog));

  /* restore main dialog sensitivity and cursor */
  gdk_window_set_cursor (GTK_WIDGET (mainDlg->dlg)->window, NULL);
  gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->dlg), TRUE);

  /* free memory */
  if (mainDlg->save_error)
    {
      g_error_free (mainDlg->save_error);
      mainDlg->save_error = NULL;
    }
  g_free (mainDlg->save_path);
  mainDlg->save_path = NULL;

  return FALSE;
}

gpointer
scr_async_save (gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  gboolean save_successful;
  
  save_successful = scr_shot_save (mainDlg->shot, mainDlg->save_path, &mainDlg->save_error);

  if (save_successful)
    g_idle_add (scr_save_done, data);
  else
    g_idle_add (scr_save_failed, data);
  
  return NULL;
}

void
scr_main_ok_clicked (GtkWidget *widget, gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  GdkCursor *cursor;
  gboolean save, upload;

  gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->dlg), FALSE);

  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (GTK_WIDGET (mainDlg->dlg)->window, cursor);
  gdk_cursor_unref (cursor);

  g_object_get (mainDlg->dest[DEST_FILE], "active", &save, NULL);
  g_object_get (mainDlg->dest[DEST_UPLOAD], "active", &upload, NULL);

  if (upload)
    {
      gint result = 
        mainDlg->cfg->warning_disabled ? GTK_RESPONSE_OK : gtk_dialog_run (mainDlg->upload_dlg);
    
      gtk_widget_hide (GTK_WIDGET (mainDlg->upload_dlg));

      switch (result)
        {
	  case GTK_RESPONSE_OK:
	    /* using a pulsing progress bar for now */
            /* gtk_progress_bar_set_fraction (mainDlg->upload_progress_bar, (gdouble) 0.0); */
	    gtk_widget_show_all (GTK_WIDGET (mainDlg->upload_progress_dlg));

            cursor = gdk_cursor_new (GDK_WATCH);
            gdk_window_set_cursor (GTK_WIDGET (mainDlg->upload_progress_dlg)->window, cursor);
            gdk_cursor_unref (cursor);

	    mainDlg->pulse_sid = g_timeout_add (200, scr_upload_pulse, data);

	    /* perform upload in a separate thread */
	    g_thread_create (scr_async_upload, data, FALSE, NULL);
	    break;

	  default:
            /* restore main dialog sensitivity and cursor */
            gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->dlg), TRUE);
            gdk_window_set_cursor (GTK_WIDGET (mainDlg->dlg)->window, NULL);
	    break;
	}
    }
  else if (save)
    {
      GtkEntry *entry = GTK_ENTRY (GTK_BIN (mainDlg->entry)->child);

      mainDlg->save_path = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

      /* save file in a separate thread */
      g_thread_create (scr_async_save, data, FALSE, NULL);
    }

}

void
scr_dest_changed (GtkWidget *widget, gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  gboolean save, upload;
  GError *error = NULL;

  g_object_get (mainDlg->dest[DEST_FILE], "active", &save, NULL);
  g_object_get (mainDlg->dest[DEST_UPLOAD], "active", &upload, NULL);

  if (upload)
    {
      GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO, GTK_ICON_SIZE_BUTTON);

      gtk_button_set_use_stock (mainDlg->ok_button, FALSE);
      gtk_button_set_label (mainDlg->ok_button, "_Upload");
      gtk_button_set_image (mainDlg->ok_button, icon);

      gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->entry), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->browse_button), FALSE);
    }
  else if (save)
    {
      GtkWidget *icon = gtk_image_new_from_stock (GTK_STOCK_SAVE, GTK_ICON_SIZE_BUTTON);

      /* for some odd reason use-stock doesn't have any effect. set the image manually...*/
      gtk_button_set_label (mainDlg->ok_button, GTK_STOCK_SAVE);
      gtk_button_set_use_stock (mainDlg->ok_button, TRUE);
      gtk_button_set_image (mainDlg->ok_button, icon);

      gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->entry), TRUE);
      gtk_widget_set_sensitive (GTK_WIDGET (mainDlg->browse_button), TRUE);
    }
}

void
warning_changed (GtkToggleButton *cb, gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  mainDlg->cfg->warning_disabled = 
    gtk_toggle_button_get_active (cb);
}

void
scr_main_browse_clicked (GtkWidget *widget, gpointer data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;
  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Save File",
                                        GTK_WINDOW (mainDlg->dlg),
					GTK_FILE_CHOOSER_ACTION_SAVE,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      GtkEntry *entry = GTK_ENTRY (GTK_BIN (mainDlg->entry)->child);
      char *filename;
  
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      gtk_entry_set_text (entry, filename);
      g_free (filename);
    }
  
  gtk_widget_destroy (dialog);
}

gchar *
scr_gen_filename (const gchar *base_dir)
{
  gint i;
  gchar *filename;

  for (i = 1; i < MAX_SCREENSHOTS; i++)
    {
      filename = g_strdup_printf (_("%s/Screenshot-%d.png"), base_dir, i);

      if (!g_file_test (filename, G_FILE_TEST_EXISTS))
        return filename;
      else
        g_free (filename);
    }

  return g_strdup (base_dir);
}

void
scr_on_preview_expose_event (GtkWidget      *drawing_area,
                             GdkEventExpose *event,
                             gpointer        data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  gdk_draw_pixbuf (drawing_area->window,
                   drawing_area->style->white_gc,
                   mainDlg->preview_scaled,
                   event->area.x,
                   event->area.y,
                   event->area.x,
                   event->area.y,
                   event->area.width,
                   event->area.height,
                   GDK_RGB_DITHER_NORMAL,
                   0, 0);
}

void
scr_on_preview_configure_event (GtkWidget         *drawing_area,
                                GdkEventConfigure *event,
                                gpointer           data)
{
  ScrMainDialog *mainDlg = (ScrMainDialog *) data;

  if (mainDlg->preview_scaled)
    g_object_unref (G_OBJECT (mainDlg->preview_scaled));

  mainDlg->preview_scaled = gdk_pixbuf_scale_simple (mainDlg->preview,
                                                     event->width,
                                                     event->height,
                                                     GDK_INTERP_BILINEAR);
}

int
main (int argc, char **argv )
{
  GladeXML *ui;
  ScrMainDialog *mainDlg = g_new0 (ScrMainDialog, 1);
  GError *error = NULL;
  const gchar *home_dir;
  const gchar *home_volatile_dir;
  gchar *screenshots_dir;
  gchar *filename, *filename_utf8;
  GtkListStore *list_store;
  GtkTreeIter path_iter;

  bindtextdomain (GETTEXT_PACKAGE, GPESCAPLOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  if (!g_thread_supported ())
    g_thread_init (NULL);

  gtk_init (&argc, &argv);

  mainDlg->cfg = g_malloc0 (sizeof (t_gpe_scap_cfg));
  load_config (mainDlg->cfg);
  
  ui = glade_utils_glade_xml_new ("main_dlg");
  mainDlg->dlg = GTK_DIALOG (glade_utils_glade_xml_get_widget (ui, "main_dlg"));
  mainDlg->preview_area = GTK_DRAWING_AREA (glade_utils_glade_xml_get_widget (ui, "drawingarea1"));
  mainDlg->dest[DEST_FILE] = GTK_RADIO_BUTTON (glade_utils_glade_xml_get_widget (ui, "radiobutton1"));
  mainDlg->dest[DEST_UPLOAD] = GTK_RADIO_BUTTON (glade_utils_glade_xml_get_widget (ui, "radiobutton2"));
  mainDlg->entry = GTK_COMBO_BOX_ENTRY (glade_utils_glade_xml_get_widget (ui, "comboboxentry1"));
  mainDlg->browse_button = GTK_BUTTON (glade_utils_glade_xml_get_widget (ui, "button3"));
  mainDlg->ok_button = GTK_BUTTON (glade_utils_glade_xml_get_widget (ui, "button2"));
  mainDlg->cancel_button = GTK_BUTTON (glade_utils_glade_xml_get_widget (ui, "button1"));

  /* standard window signals */
  g_signal_connect (G_OBJECT (mainDlg->dlg), "delete-event",
                    G_CALLBACK (scr_delete_event), NULL);

  g_signal_connect (G_OBJECT (mainDlg->dlg), "destroy",
                    G_CALLBACK (scr_quit), NULL);

  /* preview drawing */
  g_signal_connect (G_OBJECT (mainDlg->preview_area), "expose-event",
                    G_CALLBACK (scr_on_preview_expose_event), mainDlg);
  g_signal_connect (G_OBJECT (mainDlg->preview_area), "configure-event",
                    G_CALLBACK (scr_on_preview_configure_event), mainDlg);

  /* remaining gui elements */
  glade_xml_signal_connect_data (ui,
                                 "on_radiobutton2_toggled",
                                 G_CALLBACK (scr_dest_changed),
                                 mainDlg);

  glade_xml_signal_connect_data (ui,
                                 "on_button1_clicked",
                                 G_CALLBACK (scr_quit),
                                 mainDlg);
  
  glade_xml_signal_connect_data (ui,
                                 "on_button2_clicked",
                                 G_CALLBACK (scr_main_ok_clicked),
                                 mainDlg);
  
  glade_xml_signal_connect_data (ui,
                                 "on_button3_clicked",
                                 G_CALLBACK (scr_main_browse_clicked),
                                 mainDlg);
  
  list_store = gtk_list_store_new (NUM_PATH_COLUMNS,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING);
  g_object_set (GTK_COMBO_BOX (mainDlg->entry),
                "model", list_store,
		NULL);
  g_object_set (GTK_COMBO_BOX_ENTRY (mainDlg->entry),
                "text-column", PATH_VIEW_COLUMN,
		NULL);

  /* populate combobox with filename suggestions. */
  gtk_list_store_clear (list_store);

  home_dir = g_get_home_dir ();

  filename = scr_gen_filename (home_dir);
  filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
  gtk_list_store_append (list_store, &path_iter);
  gtk_list_store_set (list_store, &path_iter,
		      PATH_VIEW_COLUMN, filename_utf8,
		      PATH_SYSTEM_COLUMN, filename,
		      -1);
  g_free (filename);
  g_free (filename_utf8);

  screenshots_dir = g_strdup_printf (_("%s/Screenshots"), home_dir);
  if (g_file_test (screenshots_dir, G_FILE_TEST_IS_DIR))
    {
      filename = scr_gen_filename (screenshots_dir);
      filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
      gtk_list_store_append (list_store, &path_iter);
      gtk_list_store_set (list_store, &path_iter,
                          PATH_VIEW_COLUMN, filename_utf8,
                          PATH_SYSTEM_COLUMN, filename,
                          -1);
      g_free (filename);
      g_free (filename_utf8);
    }

  g_free (screenshots_dir);

  home_volatile_dir = g_getenv ("HOME_VOLATILE");
  if (home_volatile_dir)
    {
      filename = scr_gen_filename (home_volatile_dir);
      filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
      gtk_list_store_append (list_store, &path_iter);
      gtk_list_store_set (list_store, &path_iter,
                          PATH_VIEW_COLUMN, filename_utf8,
                          PATH_SYSTEM_COLUMN, filename,
                          -1);
      g_free (filename);
      g_free (filename_utf8);
    }

  gtk_combo_box_set_active (GTK_COMBO_BOX (mainDlg->entry), 0);

  mainDlg->shot = scr_shot_new ();
  mainDlg->preview = scr_shot_get_preview (mainDlg->shot, SCR_SCALING_FACTOR);
  gtk_widget_set_size_request (GTK_WIDGET (mainDlg->preview_area),
                               gdk_pixbuf_get_width (mainDlg->preview),
			       gdk_pixbuf_get_height (mainDlg->preview));

#ifndef DESKTOP_BUILD
  gtk_window_set_type_hint (GTK_WINDOW (mainDlg->dlg), GDK_WINDOW_TYPE_HINT_NORMAL);
#endif /* DESKTOP_BUILD */
  gtk_widget_show_all (GTK_WIDGET (mainDlg->dlg));

  ui = glade_utils_glade_xml_new ("upload_dlg");
  mainDlg->upload_dlg = GTK_DIALOG (glade_utils_glade_xml_get_widget (ui, "upload_dlg"));
  gtk_window_set_transient_for (GTK_WINDOW (mainDlg->upload_dlg), GTK_WINDOW (mainDlg->dlg));
  mainDlg->disable_warning_cb = GTK_TOGGLE_BUTTON (glade_utils_glade_xml_get_widget (ui, "cb_disable_warning"));
  gtk_toggle_button_set_active (mainDlg->disable_warning_cb, mainDlg->cfg->warning_disabled);   
  g_signal_connect (G_OBJECT (mainDlg->disable_warning_cb), "toggled",
                                 G_CALLBACK (warning_changed),
                                 mainDlg);

  ui = glade_utils_glade_xml_new ("upload_progress_dlg");
  mainDlg->upload_progress_dlg = GTK_DIALOG (glade_utils_glade_xml_get_widget (ui, "upload_progress_dlg"));
  gtk_window_set_transient_for (GTK_WINDOW (mainDlg->upload_progress_dlg), GTK_WINDOW (mainDlg->dlg));
  mainDlg->upload_progress_bar = GTK_PROGRESS_BAR (glade_utils_glade_xml_get_widget (ui, "progressbar1"));

  gtk_main ();

  save_config (mainDlg->cfg); /* todo: show returned mesage on failure */
  g_free (mainDlg->cfg);
  
  return 0;
}
