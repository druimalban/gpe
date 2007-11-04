/* import-export.c - Import and export functionality.
   Copyright (C) 2006, 2007 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <string.h>
#include <gpe/errorbox.h>
#include <gpe/vcal.h>
#include <gpe/vevent.h>
#include <mimedir/mimedir-vcal.h>

#ifdef IS_HILDON


#if HILDON_VER == 1
#include <hildon/hildon-file-chooser-dialog.h>
#else
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif /* HILDON_VER */

#endif

#include "import-export.h"
#include "calendar-edit-dialog.h"
#include "calendars-widgets.h"
#include "globals.h"

#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME   "org.handhelds.gpe.irda"
#endif /* USE_DBUS */

static void
new_calendar_clicked (GtkButton *button, gpointer user_data)
{
  GtkWidget *w = calendar_edit_dialog_new (NULL);
  gtk_window_set_transient_for
    (GTK_WINDOW (w),
     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (button))));

  if (gtk_dialog_run (GTK_DIALOG (w)) == GTK_RESPONSE_ACCEPT)
    {
      EventCalendar *ec
	= calendar_edit_dialog_get_calendar (CALENDAR_EDIT_DIALOG (w));
      if (ec)
	calendars_combo_box_set_active (user_data, ec);
    }

  gtk_widget_destroy (w);
}

gboolean
cal_import_from_files (EventCalendar *ec, const char *files[],
		       GError **gerror)
{
  GtkBox *box;
  GtkWidget *combo;
  GtkWidget *filesel = NULL;

  if (! files)
    /* No files were provided, prompt for some.  */
    {
#if IS_HILDON
      filesel = hildon_file_chooser_dialog_new (GTK_WINDOW (main_window), 
                        GTK_FILE_CHOOSER_ACTION_OPEN);
      gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (filesel), TRUE);
#else
      filesel = gtk_file_selection_new (_("Choose file"));
      gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (filesel), TRUE);
#endif

      gtk_window_set_transient_for (GTK_WINDOW (filesel), 
				    GTK_WINDOW (main_window));
    }

  if (! ec)
    /* We need to know into which calendar we should place the events.
       Create a widget.  */
    {
      box = GTK_BOX (gtk_hbox_new (FALSE, 3));
      gtk_widget_show (GTK_WIDGET (box));

      /* We cannot integrate the calendar selection into Hildon's file
	 chooser.  We'll prompt for the calendar afterwards with a
	 separate dialog.  */
#if ! IS_HILDON
      if (! files)
         gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (filesel)->main_vbox),
			    GTK_WIDGET (box), FALSE, FALSE, 0);
#endif

      GtkWidget *w = gtk_button_new_from_stock (GTK_STOCK_NEW);
      gtk_widget_show (w);
      gtk_box_pack_end (box, w, FALSE, FALSE, 0);

      combo = calendars_combo_box_new (event_db);
      gtk_widget_show (combo);
      gtk_box_pack_end (box, combo, FALSE, FALSE, 0);

      g_signal_connect (G_OBJECT (w), "clicked",
			G_CALLBACK (new_calendar_clicked), combo);

      w = gtk_label_new (_("Import into calendar: "));
      gtk_widget_show (GTK_WIDGET (w));
      gtk_box_pack_end (box, w, FALSE, FALSE, 0);
    }

  if (! files)
    /* Run the file chooser.  */
    {
      if (gtk_dialog_run (GTK_DIALOG (filesel)) != GTK_RESPONSE_OK)
	{
	  gtk_widget_destroy (filesel);
	  return FALSE;
	}
      gtk_widget_hide (filesel); 

#if IS_HILDON
      files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (filesel));
#else		
      files = gtk_file_selection_get_selections (GTK_FILE_SELECTION (filesel));
#endif
    }

  GtkWidget *calsel = NULL;
  if (! ec && (! filesel || 0
#ifdef IS_HILDON
	       + 1
#endif
	       ))
    /* Prompt for the calendar.  */
    {
      char *title = g_strdup_printf (_("Select Calendar for %s%s"),
				     files[0], files[1] ? "..." : "");
      calsel = gtk_dialog_new_with_buttons
	(title, GTK_WINDOW (main_window),
	 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
	 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
	 GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
	 0);
      g_free (title);

      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (calsel)->vbox),
			  GTK_WIDGET (box), FALSE, FALSE, 0);
      if (gtk_dialog_run (GTK_DIALOG (calsel)) != GTK_RESPONSE_ACCEPT)
        {
          gtk_widget_destroy (calsel);
          return FALSE;
        }
    }

  if (! ec)
    ec = calendars_combo_box_get_active (GTK_COMBO_BOX (combo));
  g_object_ref (ec);
  if (filesel)
    gtk_widget_destroy (filesel);
  if (calsel)
    gtk_widget_destroy (calsel);

  gchar *errstr = NULL;
  int i;
  for (i = 0; files[i]; i ++)
    {
      GError *error = NULL;
      GList *callist = mimedir_vcal_read_file (files[i], &error);
      if (error) 
        {
          gchar *tmp;
          tmp = g_strdup_printf ("%s%s%s: %s",
				 errstr ?: "", errstr ? "\n" : "",
				 files[i], error->message);
          g_free (errstr);
          errstr = tmp;
          g_error_free (error);
          continue;
        }

      if (! cal_import_from_vmimedir (ec, callist, &error))
	{
	  if (! errstr)
	    errstr = g_strdup (error->message);
	  else
	    {
	      char *tmp = g_strjoin ("\n", errstr, error->message, NULL);
	      g_free (errstr);
	      errstr = tmp;
	    }

	  g_error_free (error);
	}

      /* Cleanup */
      mimedir_vcal_free_list (callist);
    }

  g_object_unref (ec);

  if (errstr)
    {
      g_set_error (gerror, ERROR_DOMAIN (), 0, "%s", errstr);
      g_free (errstr);
      return FALSE;
    }
  return TRUE;
}

/* This is just a wrapper for import_vcal which displays the results
   to the user.  */
void
cal_import_dialog (EventCalendar *ec, const char *files[])
{
  GError *error = NULL;
  int res = cal_import_from_files (ec, files, &error);
  if (! res && ! error)
    /* Abort.  */
    return;

  GtkWidget *feedbackdlg = gtk_message_dialog_new
    (GTK_WINDOW (main_window),
     GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
     GTK_BUTTONS_OK, error ? error->message : _("Import successful"));

  if (error)
    g_error_free (error);
    
  gtk_dialog_run (GTK_DIALOG (feedbackdlg));
  gtk_widget_destroy (feedbackdlg);
}

/* THING is either an Event or a Calendar.  */
static void
save_as_dialog (GObject *thing)
{
  GtkWidget *filesel;
  const gchar *filename;

  g_object_ref (thing);

  char *summary;
  if (IS_EVENT (thing))
    summary = event_get_summary (EVENT (thing), NULL);
  else
    summary = event_calendar_get_title (EVENT_CALENDAR (thing), NULL);

  char *suggestion = g_strdup_printf ("%s.ics",
				      summary && *summary ? summary : "calendar");
  char *s = suggestion;
  while ((s = strchr (s, ' ')))
    *s = '_';

#ifdef IS_HILDON	
  filesel = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(filesel), suggestion);
#else
  s = g_strdup_printf (_("Save %s as..."), summary);
  filesel = gtk_file_selection_new (s);
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), suggestion);
#endif
  
  gtk_widget_show (filesel);
  if (gtk_dialog_run (GTK_DIALOG (filesel)) == GTK_RESPONSE_OK)
    {
#ifdef IS_HILDON
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
#else
      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
#endif
      GError *error = NULL;
      int res;
      if (IS_EVENT_CALENDAR (thing))
	res = cal_export_to_file (EVENT_CALENDAR (thing), filename, &error);
      else
	res = event_export_to_file (EVENT (thing), filename, &error);
      if (! res)
	{
	  gpe_error_box_fmt (_("Saving %s: %s"), summary, error->message);
	  g_error_free (error);
	  g_free (summary);
	}
    }
  
  g_free (summary);
  gtk_widget_destroy (filesel);
  g_object_unref (thing);
  return;
}

void
event_export_dialog (Event *ev)
{
  save_as_dialog (G_OBJECT (ev));
}

void
cal_export_dialog (EventCalendar *ec)
{
  save_as_dialog (G_OBJECT (ec));
}


/* event_export_to_file is in vcal.c.  */

/* Initialize dbus if not already initialized.  */
static void
dbus_init (void)
{
#ifdef USE_DBUS
  static int init;

  if (! init)
    {
      init = 1;

      DBusError error;
      dbus_error_init (&error);

      connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
      if (connection)
	dbus_connection_setup_with_g_main (connection, NULL);
    }
#endif /* USE_DBUS */
}

gboolean
export_bluetooth_available (void)
{
#ifdef USE_DBUS
  dbus_init ();

  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  r = dbus_bus_name_has_owner (connection, BLUETOOTH_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, BLUETOOTH_SERVICE_NAME, NULL);
#endif

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

gboolean
export_irda_available (void)
{
#ifdef USE_DBUS
  dbus_init ();

  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  r = dbus_bus_name_has_owner (connection, IRDA_SERVICE_NAME, NULL);
#else
  r = dbus_bus_service_exists (connection, IRDA_SERVICE_NAME, NULL);
#endif

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

void
vcal_do_send_bluetooth (Event *event)
{
#ifdef USE_DBUS
  dbus_init ();

  gchar *vcal;
  DBusMessage *message;
  gchar *filename, *mimetype;
#ifndef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  DBusMessageIter iter;
#endif
  vcal = event_export_as_string (event);

  message = dbus_message_new_method_call (BLUETOOTH_SERVICE_NAME,
					  "/org/handhelds/gpe/bluez/OBEX",
					  BLUETOOTH_SERVICE_NAME ".OBEX",
					  "ObjectPush");

  filename = "GPE.vcf";
  mimetype = "application/x-vcal";

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_append_args (message, DBUS_TYPE_STRING, &filename,
			    DBUS_TYPE_STRING, &mimetype,
			    DBUS_TYPE_ARRAY, DBUS_TYPE_BYTE, 
			    &vcal, strlen (vcal), DBUS_TYPE_INVALID);
#else
  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, filename);
  dbus_message_iter_append_string (&iter, mimetype);
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));
#endif

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}

void
vcal_do_send_irda (Event *event)
{
#ifdef USE_DBUS
  dbus_init ();

  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;

  vcal = event_export_as_string (event);

  message = dbus_message_new_method_call (IRDA_SERVICE_NAME,
					  "/org/handhelds/gpe/irda/OBEX",
					  IRDA_SERVICE_NAME ".OBEX",
					  "ObjectPush");

#ifdef HAVE_DBUS_MESSAGE_ITER_GET_BASIC
  dbus_message_iter_init_append (message, &iter);

  dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, "GPE.vcf");
  dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, "application/x-vcal");
  dbus_message_iter_append_fixed_array (&iter, DBUS_TYPE_BYTE, vcal, strlen (vcal));
#else
  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, "GPE.vcf");
  dbus_message_iter_append_string (&iter, "application/x-vcal");
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));
#endif

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}
