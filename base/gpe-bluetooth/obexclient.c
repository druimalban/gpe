/*
 * Copyright (C) 2004, 2006 Philip Blundell <philb@gnu.org>
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
#include "obex-glib.h"
#include "main.h"
#include "sdp.h"

#define _(x) gettext(x)

typedef void (*obex_push_callback)(gboolean success, gpointer data);

enum req_state
  {
    STATE_CONNECT,
    STATE_PUT,
    STATE_DISCONNECT
  };

struct obex_push_req
{
  bdaddr_t bdaddr;
  const gchar *filename;
  const gchar *mimetype;
  const gchar *data;
  size_t len;
  obex_t *obex;
  enum req_state state;
  GtkWidget *progress_dialog;

  GCallback callback;
  gpointer cb_data;
};

static GStaticMutex active_reqs_mutex = G_STATIC_MUTEX_INIT;
static GList *active_reqs;
static struct bt_service_desc obex_service_desc;

static void
run_callback (struct obex_push_req *req, gboolean result)
{
  obex_push_callback cb;
  
  cb = (obex_push_callback)req->callback;

  (*cb)(result, req->cb_data);

  g_free (req);
}

static gboolean
find_obex_service (bdaddr_t *bdaddr, int *port)
{
  sdp_list_t *attrid, *search, *seq, *next;
  uint32_t range = 0x0000ffff;
  int status = -1;
  sdp_session_t *sess;
  gboolean found = FALSE;

  attrid = sdp_list_append (0, &range);
  search = sdp_list_append (0, &obex_service_desc.uuid);
  sess = sdp_connect (BDADDR_ANY, bdaddr, 0);
  if (!sess) 
    return FALSE;

  status = sdp_service_search_attr_req (sess, search, SDP_ATTR_REQ_RANGE, attrid, &seq);
  if (status) 
    {
      sdp_close (sess);
      return FALSE;
    }

  for (; seq; seq = next)
    {
      sdp_record_t *svcrec = (sdp_record_t *) seq->data;

      if (sdp_find_rfcomm (svcrec, port))
	found = TRUE;

      next = seq->next;
      free (seq);
      sdp_record_free (svcrec);
    }

  sdp_close (sess);

  return found;
}

static struct obex_push_req *
find_request (obex_t *obex)
{
  struct obex_push_req *req = NULL;
  GList *i;

  g_static_mutex_lock (&active_reqs_mutex);
  for (i = active_reqs; i; i = i->next)
    {
      struct obex_push_req *r = i->data;
      if (r->obex == obex)
	{
	  req = r;
	  break;
	}
    }
  g_static_mutex_unlock (&active_reqs_mutex);
  
  return req;
}

static gboolean
obex_state_machine (struct obex_push_req *req)
{
  obex_object_t *object;
  obex_headerdata_t hdd;
  uint8_t unicode_buf[200];
  int namebuf_len;

  switch (req->state)
    {
    case STATE_CONNECT:
      gdk_threads_enter ();
      bt_progress_dialog_update (req->progress_dialog, _("Sending data"));
      gdk_flush ();
      gdk_threads_leave ();
      object = OBEX_ObjectNew (req->obex, OBEX_CMD_PUT);
      namebuf_len = OBEX_CharToUnicode (unicode_buf, req->filename, sizeof (unicode_buf));
      hdd.bs = unicode_buf;
      OBEX_ObjectAddHeader (req->obex, object, OBEX_HDR_NAME, hdd, namebuf_len, 0);
      hdd.bq4 = req->len;
      OBEX_ObjectAddHeader (req->obex, object, OBEX_HDR_LENGTH, hdd, sizeof (uint32_t), 0);
      hdd.bs = req->data;
      OBEX_ObjectAddHeader (req->obex, object, OBEX_HDR_BODY, hdd, req->len, 0);
      OBEX_Request (req->obex, object);
      req->state = STATE_PUT;
      break;
    case STATE_PUT:
      object = OBEX_ObjectNew (req->obex, OBEX_CMD_DISCONNECT);
      OBEX_Request (req->obex, object);
      req->state = STATE_DISCONNECT;
      break;
    case STATE_DISCONNECT:
      gdk_threads_enter ();
      gtk_widget_destroy (req->progress_dialog);
      gdk_flush ();
      gdk_threads_leave ();
      g_static_mutex_lock (&active_reqs_mutex);
      active_reqs = g_list_remove (active_reqs, req);
      g_static_mutex_unlock (&active_reqs_mutex);
      obex_disconnect_input (req->obex);
      OBEX_Cleanup (req->obex);
      run_callback (req, TRUE);
      break;
    }

  return FALSE;
}

static void
obex_event (obex_t *obex, obex_object_t *old_object, int mode, int event, int obex_cmd, int obex_rsp)
{
  struct obex_push_req *req;

  req = find_request (obex);

  switch (event)
    {
    case OBEX_EV_PROGRESS:
      /* do nothing */
      break;
    case OBEX_EV_REQDONE:
      /* At this point, libopenobex is still threaded and will not accept a new request.
	 Schedule a timeout to start the next phase.  */
      g_timeout_add (1, (GSourceFunc)obex_state_machine, req);
      break;
    default:
      printf ("unknown obex client event %d\n", event);
    }
}

static void
obex_do_connect (gpointer data)
{
  struct obex_push_req *req = (struct obex_push_req *)data;
  int port;
  gchar *text;
  GtkWidget *w;
  obex_object_t *object;
  int r;
  gboolean use_perror = TRUE;
  
  gdk_threads_enter ();
  w = bt_progress_dialog (_("Connecting"), gpe_find_icon ("bt-logo"));
  gtk_widget_show_all (w);
  gdk_flush ();
  gdk_threads_leave ();

  if (find_obex_service (&req->bdaddr, &port) == FALSE)
    {
      text = _("Selected device lacks OBEX support");
      use_perror = FALSE;
      goto error;
    }

  req->obex = OBEX_Init (OBEX_TRANS_BLUETOOTH, obex_event, 0);
  if (req->obex == NULL)
    {
      text = _("Unable to create OBEX object");
      goto error;
    }

  if (r = BtOBEX_TransportConnect (req->obex, BDADDR_ANY, &req->bdaddr, port), r < 0)
    {
      OBEX_Cleanup (req->obex);
      text = _("Unable to connect");
      errno = r;
      goto error;
    }

  obex_connect_input (req->obex);

  req->progress_dialog = w;
  req->state = STATE_CONNECT;
  g_static_mutex_lock (&active_reqs_mutex);
  active_reqs = g_list_append (active_reqs, req);
  g_static_mutex_unlock (&active_reqs_mutex);  

  object = OBEX_ObjectNew (req->obex, OBEX_CMD_CONNECT);
  OBEX_Request (req->obex, object);
  return;

 error:
  gdk_threads_enter ();
  gtk_widget_destroy (w);
  gdk_flush ();
  if (use_perror)
    gpe_perror_box_nonblocking (text);
  else
    gpe_error_box_nonblocking (text);
  gdk_threads_leave ();
  run_callback (req, FALSE);
}

static void
cancel_clicked (GtkWidget *w, GtkWidget *dialog)
{
  struct obex_push_req *req;

  req = g_object_get_data (G_OBJECT (dialog), "req");

  gtk_widget_destroy (dialog);

  run_callback (req, FALSE);
}

static void
ok_clicked (GtkWidget *w, GtkWidget *dialog)
{
  struct obex_push_req *req;
  GtkWidget *tree_view;
  GtkTreeSelection *sel;
  GList *list, *i;
  GtkTreePath *path;
  GtkTreeModel *model;
  bdaddr_t *bdaddr;
  GtkTreeIter iter;
  GThread *thread;

  req = g_object_get_data (G_OBJECT (dialog), "req");
  tree_view = g_object_get_data (G_OBJECT (dialog), "tree_view");

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));

  list = gtk_tree_selection_get_selected_rows (sel, &model);

  if (list == NULL)
    {
      gtk_widget_destroy (dialog);
      gpe_error_box_nonblocking (_("No destination selected"));
      run_callback (req, FALSE);
      return;
    }

  path = list->data;
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, 2, &bdaddr, -1);
  memcpy (&req->bdaddr, bdaddr, sizeof (*bdaddr));

  for (i = list; i; i = i->next)
    {
      path = i->data;
      gtk_tree_path_free (path);
    }

  g_list_free (list);

  thread = g_thread_create ((GThreadFunc) obex_do_connect, req, FALSE, NULL);

  if (thread == NULL)
    run_callback (req, FALSE);
  
  gtk_widget_destroy (dialog);
}

static void
free_bdaddrs (GtkWidget *w, GtkListStore *list_store)
{
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store), &iter))
    {
      do 
	{
	  bdaddr_t *bdaddr;
	  gchar *str;
	  GdkPixbuf *pixbuf;

	  gtk_tree_model_get (GTK_TREE_MODEL (list_store), &iter, 0, &pixbuf, 1, &str, 2, &bdaddr, -1);
	  g_free (bdaddr);
	  g_free (str);
	  gdk_pixbuf_unref (pixbuf);
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (list_store), &iter));
    }
}

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
      memcpy (bdaddr, &(info+i)->bdaddr, sizeof (*bdaddr));

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

  g_object_set_data (G_OBJECT (dialog), "req", req);
  g_object_set_data (G_OBJECT (dialog), "tree_view", tree_view);
  g_signal_connect (G_OBJECT (ok_button), "clicked", G_CALLBACK (ok_clicked), dialog);
  g_signal_connect (G_OBJECT (cancel_button), "clicked", G_CALLBACK (cancel_clicked), dialog);  

  g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (free_bdaddrs), list_store);

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

struct bt_service_obex
{
  struct bt_service service;
  struct bt_device *bd;
};

static struct bt_service *
obex_scan (sdp_record_t *rec, struct bt_device *bd)
{
  struct bt_service_obex *s;

  s = g_malloc (sizeof (*s));

  s->service.desc = &obex_service_desc;
  s->bd = bd;

  return (struct bt_service *)s;
}

static void
send_vcard_done (gboolean success, void *data)
{
  g_free (data);
}

static gboolean
really_send_my_vcard (struct bt_service_obex *svc)
{
  struct obex_push_req *req;
  gchar *filename;
  gchar *data;
  gsize length;
  GError *error;

  filename = g_strdup_printf ("%s/.gpe/user.vcf", g_get_home_dir ());

  if (access (filename, R_OK) && errno == ENOENT)
    {
      if (system ("gpe-contacts -v"))
	{
	  g_free (filename);
	  return FALSE;
	}
    }

  error = NULL;
  if (!g_file_get_contents (filename, &data, &length, &error))
    {
      gdk_threads_enter ();
      gpe_error_box (error->message);
      gdk_threads_leave ();
      g_error_free (error);
      g_free (filename);
      return FALSE;
    }

  req = g_new (struct obex_push_req, 1);
  baswap (&req->bdaddr, &svc->bd->bdaddr);
  req->filename = "user.vcf";
  req->mimetype = "application/x-vcard";
  req->data = data;
  req->len = length;
  req->callback = send_vcard_done;
  req->cb_data = data;

  obex_do_connect (req);
}

static void
send_my_vcard (GtkWidget *w, struct bt_service_obex *svc)
{
  GThread *thread;

  thread = g_thread_create ((GThreadFunc)really_send_my_vcard, svc, FALSE, NULL);
}

static void
obex_popup_menu (struct bt_service *svc, GtkWidget *menu)
{
  struct bt_service_obex *obex;
  GtkWidget *w;
  obex = (struct bt_service_obex *)svc;

  w = gtk_menu_item_new_with_label (_("Send my vCard"));
  g_signal_connect (G_OBJECT (w), "activate", G_CALLBACK (send_my_vcard), svc);

  gtk_widget_show (w);
  gtk_menu_append (GTK_MENU (menu), w);
}

void
obex_client_init (void)
{
  sdp_uuid16_create (&obex_service_desc.uuid, OBEX_OBJPUSH_SVCLASS_ID);
  obex_service_desc.scan = obex_scan;
  obex_service_desc.popup_menu = obex_popup_menu;

  service_desc_list = g_slist_prepend (service_desc_list, &obex_service_desc);
}
