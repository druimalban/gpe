/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *               2005 Florian Boor <florian@kernelconcepts.de> 
 * Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>

#include "export-vcal.h"
#include "globals.h"

#include <gpe/vevent.h>
#include <gpe/errorbox.h>
#include <mimedir/mimedir.h>

#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME   "org.handhelds.gpe.irda"
#endif /* USE_DBUS */

#ifdef IS_HILDON /* Hildon includes */
#include <hildon-fm/hildon-widgets/hildon-file-chooser-dialog.h>
#endif


static gchar *
export_to_vcal (Event *event)
{
  MIMEDirVEvent *vevent;
  gchar *str;
    
  vevent = vevent_from_event_t (event);

  str = mimedir_vevent_write_to_string (vevent);
  g_object_unref (vevent);

  return str;
}

void
vcal_do_send_bluetooth (Event *event)
{
#ifdef USE_DBUS
  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;
  gchar *filename, *mimetype;

  vcal = export_to_vcal (event);

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
  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;

  vcal = export_to_vcal (event);

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

gboolean
save_to_file(Event *event, const gchar *filename)
{
  gchar *vcal;
  int fd;
  
  vcal = export_to_vcal (event);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    goto error;

  if (write (fd, vcal, strlen (vcal)) < strlen (vcal))
    goto error;
  
  if (close (fd))
    goto error;

  g_free (vcal);
  return FALSE;
  
 error:
  g_free (vcal);
  return TRUE;
}


void
vcal_do_save (Event *event)
{
  GtkWidget *filesel;
  const gchar *filename;

#ifdef IS_HILDON	
  filesel = hildon_file_chooser_dialog_new(NULL, GTK_FILE_CHOOSER_ACTION_SAVE);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(filesel), "GPE.vcal");
#else
  filesel = gtk_file_selection_new (_("Save as..."));
  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), "GPE.vcal");
#endif
  
  gtk_widget_show_all (filesel);
  if (gtk_dialog_run(GTK_DIALOG(filesel)) == GTK_RESPONSE_OK)
    {
#ifdef IS_HILDON
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filesel));
#else
      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
#endif
      if (save_to_file(event, filename))
        goto error;
    }
  
  gtk_widget_destroy (filesel);
  return;

 error:
  gtk_widget_destroy (filesel);
  gpe_perror_box (filename);
}

gboolean
export_bluetooth_available (void)
{
#ifdef USE_DBUS
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
vcal_export_init (void)
{
#ifdef USE_DBUS
  DBusError error;

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    dbus_connection_setup_with_g_main (connection, NULL);
#endif /* USE_DBUS */
}
