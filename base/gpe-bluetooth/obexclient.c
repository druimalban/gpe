/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <libintl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gtk/gtk.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#include <openobex/obex.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include "obexclient.h"
#include "main.h"

#define _(x) gettext(x)

struct obex_push_req
{
  const gchar *filename;
  const gchar *mimetype;
  const gchar *data;
  size_t len;

  GCallback callback;
  gpointer cb_data;
};

static gboolean
obex_choose_destination (gpointer data)
{
  struct obex_push_req *req;
  GtkWidget *w;
  int num_rsp, length, flags;
  inquiry_info *info = NULL;
  int i, dd;
  GtkListStore *list_store;
  GtkWidget *tree_view;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  const char *text;
  GtkWidget *dialog, *ok_button, *cancel_button;

  req = (struct obex_push_req *)data;

  gdk_threads_enter ();
  w = bt_progress_dialog (_("Scanning for devices"), gpe_find_icon ("bt-logo"));
  gtk_widget_show_all (w);
  gdk_flush ();
  gdk_threads_leave ();

  length  = 4;  /* ~10 seconds */
  num_rsp = 10;
  flags = 0;

  num_rsp = hci_inquiry (-1, length, num_rsp, NULL, &info, flags);
  if (num_rsp < 0) 
    {
      text = _("Inquiry failed");
      goto error;
    }

  dd = hci_open_dev (0/*dev_id*/);
  if (dd < 0) 
    {
      free (info);
      text = _("HCI device open failed");
      goto error;
    }

  gdk_threads_enter ();
  gtk_widget_destroy (w);
  list_store = gtk_list_store_new (3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER);
  gdk_threads_leave ();

  for (i = 0; i < num_rsp; i++)
    {
      GtkTreeIter iter;
      bdaddr_t *bdaddr;
      gchar *str;
      char name[248];
      int class;
      GdkPixbuf *pixbuf = NULL;
      const gchar *pixbuf_name;

      memset (name, 0, sizeof (name));
      if (hci_read_remote_name (dd, &(info+i)->bdaddr, sizeof (name), name, 25000) < 0)
	strcpy (name, _("unknown"));

      class = ((info+i)->dev_class[2] << 16) | ((info+i)->dev_class[1] << 8) | (info+i)->dev_class[0];

      pixbuf_name = icon_name_for_class (class);
      pixbuf = gpe_find_icon_scaled (pixbuf_name, GTK_ICON_SIZE_MENU);
      gdk_pixbuf_ref (pixbuf);
      
      str = g_strdup (name);

      bdaddr = g_malloc (sizeof (*bdaddr));
      baswap (bdaddr, &(info+i)->bdaddr);

      gdk_threads_enter ();			// probably not required
      gtk_list_store_append (list_store, &iter);
      gtk_list_store_set (list_store, &iter, 
			  0, pixbuf,
			  1, str,
			  2, bdaddr,
			  -1);
      gdk_threads_leave ();
    }

  close (dd);
  free (info);

  gdk_threads_enter ();
  dialog = gtk_dialog_new ();
  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (list_store));

  renderer = gtk_cell_renderer_pixbuf_new ();
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						  "pixbuf", 0,
						  NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);

  renderer = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes (NULL, renderer,
						  "text", 1,
						  NULL);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (tree_view), col, -1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), tree_view, TRUE, TRUE, 0);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  ok_button = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), cancel_button, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), ok_button, FALSE, FALSE, 0);

  gtk_window_set_default_size (GTK_WINDOW (dialog), 240, 320);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Select destination"));
  gpe_set_window_icon (dialog, "bt-logo");

  gtk_widget_show_all (dialog);

  gdk_flush ();
  gdk_threads_leave ();

  return TRUE;

 error:
  gdk_threads_enter ();
  gtk_widget_destroy (w);
  gpe_perror_box_nonblocking (text);
  gdk_threads_leave ();
  return FALSE;
}

gboolean
obex_object_push (const gchar *filename, const gchar *mimetype, const gchar *data, size_t len,
		  GCallback callback, gpointer cb_data)
{
  struct obex_push_req *req;
  GThread *thread;

  req = g_new (struct obex_push_req, 1);

  req->filename = filename;
  req->mimetype = mimetype;
  req->data = data;
  req->len = len;
  req->callback = callback;
  req->cb_data = cb_data;

  thread = g_thread_create ((GThreadFunc) obex_choose_destination, req, FALSE, NULL);

  if (thread == NULL)
    {
      g_free (req);
      return FALSE;
    }

  return TRUE;
}

DBusHandlerResult
obex_client_handle_dbus_request (DBusConnection *connection, DBusMessage *message)
{
  DBusMessageIter iter;
  DBusMessage *reply;
  int type;

  dbus_message_iter_init (message, &iter);
 
  reply = dbus_message_new_method_return (message);
  if (!reply)
    return DBUS_HANDLER_RESULT_NEED_MEMORY;

  return DBUS_HANDLER_RESULT_HANDLED;
}

struct file_push_context
{
  gchar *filename;
  gchar *data;
};

static void
file_push_done (gpointer data)
{
  struct file_push_context *c = data;

  g_free (c->filename);
  g_free (c->data);
  g_free (c);
}

static void
select_file_done (GtkWidget *w, GtkWidget *filesel)
{
  gchar *data;
  size_t len;
  gchar *filename;
  int fd;
  struct stat sb;
  struct file_push_context *c;

  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel)));
  fd = open (filename, O_RDONLY);
  if (fd < 0)
    {
      gpe_perror_box (filename);
      goto error;
    }

  if (fstat (fd, &sb))
    {
      gpe_perror_box ("stat");
      close (fd);
      goto error;
    }
  
  len = sb.st_size;
  data = g_malloc (len);
  if (read (fd, data, len) < len)
    {
      gpe_perror_box ("read");
      g_free (data);
      close (fd);
      goto error;
    }

  close (fd);

  c = g_malloc (sizeof (*c));
  c->filename = filename;
  c->data = data;

  if (obex_object_push (filename, NULL, data, len, G_CALLBACK (file_push_done), c) == FALSE)
    {
      g_free (data);
      g_free (filename);
      g_free (c);
    }
      
  gtk_widget_destroy (filesel);
  return;

 error:
  g_free (filename);
  gtk_widget_destroy (filesel);
}


void
send_file_dialog (void)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new (_("Select file"));

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
			    "clicked", G_CALLBACK (gtk_widget_destroy), filesel);

  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
		    "clicked", G_CALLBACK (select_file_done), filesel);

  gtk_widget_show_all (filesel);
}
