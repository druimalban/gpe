 /*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *               2004 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gdk/gdkx.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-mime.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>
#include <libgnomevfs/gnome-vfs-mime-info.h>
#include <libgnomevfs/gnome-vfs-module-callback.h>
#include <libgnomevfs/gnome-vfs-standard-callbacks.h>
#include <libgnomevfs/gnome-vfs-xfer.h>
#include <libgnomevfs/gnome-vfs-types.h>
#include <libgnomevfs/gnome-vfs-application-registry.h>

#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <X11/Xlib.h>
#include <gpe/infoprint.h>
#include <gpe/dirbrowser.h>

#include "main.h"

#define _(x) dgettext(PACKAGE, x)

typedef struct
{
  gchar *destination;
  gint col;
  GtkWidget *view;
}t_movedata;
static t_movedata md;

GList *file_clipboard = NULL;

static GtkWidget *progress_dialog = NULL;
static gboolean abort_transfer = FALSE;
static GList *dirlist = NULL;

extern GtkWidget *window;

/* file handling callbacks */

static void
progress_response(GtkDialog *dialog, gint arg1, gpointer user_data)
{
  /* check and break */
  abort_transfer = TRUE;  
}

static GtkWidget*
progress_dialog_create(gchar *title)
{
  GtkWidget *dialog;
  GtkWidget *progress;
  dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
         progress = gtk_progress_bar_new();
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
                            progress,FALSE,TRUE,0);
         g_object_set_data(G_OBJECT(dialog),"bar",progress);
         g_signal_connect(G_OBJECT(dialog),"response",
                          G_CALLBACK(progress_response),NULL);
         gtk_widget_show_all(dialog);
  return dialog;
}

static gint 
transfer_callback(GnomeVFSXferProgressInfo *info, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *label;
  gchar *text;
  int response;
  gboolean applytoall;

  if (abort_transfer)
  {
    gtk_widget_destroy(progress_dialog);
    progress_dialog = NULL;
    abort_transfer = FALSE;
    return 0;
  }
  switch (info->status)
  {
	  case GNOME_VFS_XFER_PROGRESS_STATUS_OK:
          if (progress_dialog == NULL)
              progress_dialog = progress_dialog_create("Transfer progress");
          else if (info->bytes_total)
          {
            GtkWidget *bar = g_object_get_data(G_OBJECT(progress_dialog),"bar");
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar),
                                          (gdouble)info->total_bytes_copied
                                          /(gdouble)(info->bytes_total));
          }
          if (info->phase == GNOME_VFS_XFER_PHASE_COMPLETED)
          {
            gtk_widget_destroy(progress_dialog);
            progress_dialog = NULL;
          }
          gtk_main_iteration_do(FALSE);
          return 1;
	  break;
	  case GNOME_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		  /* error query dialog */
          text = g_strdup_printf ("Error: %s", 
            gnome_vfs_result_to_string(info->vfs_status));
          dialog = gtk_dialog_new_with_buttons ("Error", GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             "_Skip", GTK_RESPONSE_HELP,
             "_Retry", GTK_RESPONSE_APPLY, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
         label = gtk_label_new(text);
         g_free(text);
         gtk_misc_set_alignment(GTK_MISC(label),0,0.5);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         gtk_widget_show_all(dialog);
         response = gtk_dialog_run(GTK_DIALOG(dialog));
         gtk_widget_destroy(dialog);
         switch (response)
         {
             case GTK_RESPONSE_CANCEL: 
                 gtk_widget_destroy(progress_dialog);
                 progress_dialog = NULL;
                 return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
             break;
             case GTK_RESPONSE_HELP: 
                 return GNOME_VFS_XFER_ERROR_ACTION_SKIP;
             break;
             case GTK_RESPONSE_APPLY: 
                 return GNOME_VFS_XFER_ERROR_ACTION_RETRY;
             break;
             default: 
                 gtk_widget_destroy(progress_dialog);
                 progress_dialog = NULL;
                 return GNOME_VFS_XFER_ERROR_ACTION_ABORT;
             break;
         }
	  break;
	  case GNOME_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		 /* overwrite query dialog */
          text = g_strdup_printf ("Overwrite file:\n%s", info->target_name);
          dialog = gtk_dialog_new_with_buttons ("File exists", GTK_WINDOW (window), 
             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
             GTK_STOCK_YES, GTK_RESPONSE_YES,
             GTK_STOCK_NO, GTK_RESPONSE_NO, NULL);
         label = gtk_label_new(text);
         g_free(text);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         label = gtk_check_button_new_with_label("Apply to all");
         gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(label),FALSE);
         gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,FALSE,TRUE,0);
         gtk_widget_show_all(dialog);
         response = gtk_dialog_run(GTK_DIALOG(dialog));
         applytoall = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(label));
         gtk_widget_destroy(dialog);
         switch (response)
         {
           case GTK_RESPONSE_CANCEL:
             gtk_widget_destroy(progress_dialog);
             progress_dialog = NULL;
             return GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT;
           break;
           case GTK_RESPONSE_YES:
             if (applytoall)                 
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL;
             else
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_REPLACE;
           break;
           case GTK_RESPONSE_NO: 
               if (applytoall)
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP_ALL;
               else
                 return GNOME_VFS_XFER_OVERWRITE_ACTION_SKIP;
           break;
           default:
             gtk_widget_destroy(progress_dialog);
             progress_dialog = NULL;
             return GNOME_VFS_XFER_OVERWRITE_ACTION_ABORT;
           break;
         }
	  break;
     default:
        return 1; /* we should never get there */
     break;	  
  }
}

/* clipboard management */

static void 
clear_clipboard (void)
{
  GList *iter = file_clipboard;
  
  if (!file_clipboard) 
    return;
  
  while (iter)
  {
    g_free(iter->data);
    iter = iter->next;
  }
  g_list_free(file_clipboard);
  file_clipboard = NULL;
}

static void
clip_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
               gpointer data)
{
  FileInformation *cfi = NULL;
  gint col = (gint)data;

  gtk_tree_model_get(model, iter, col, &cfi, -1);
  if (cfi)
  {
    if (GNOME_VFS_FILE_INFO_LOCAL(cfi->vfs))
      file_clipboard = g_list_append(file_clipboard, 
                                     gnome_vfs_get_uri_from_local_path(cfi->filename));
    else
      file_clipboard = g_list_append(file_clipboard, g_strdup(cfi->filename));
  }
}

void 
copy_file_clip (GtkWidget *active_view, gint col)
{
  clear_clipboard();
  GtkTreeSelection *sel = NULL;

  if (active_view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  
  if (!sel) 
    return;
  
  gtk_tree_selection_selected_foreach(sel, 
                                     (GtkTreeSelectionForeachFunc)clip_one_file,
                                     (gpointer)col);
  
  gpe_popup_infoprint(GDK_DISPLAY(), _("File(s) copied to clipboard."));
}


void
copy_file (const gchar *src_uri_txt, const gchar *dest_uri_txt)
{
  GnomeVFSURI *uri_src = gnome_vfs_uri_new(src_uri_txt);
  GnomeVFSURI *uri_dest = gnome_vfs_uri_new(dest_uri_txt);
  GnomeVFSResult result;
  gchar *error;
  
      
  result = gnome_vfs_xfer_uri (uri_src,
                               uri_dest,
                               GNOME_VFS_XFER_FOLLOW_LINKS 
                                 | GNOME_VFS_XFER_RECURSIVE 
                                 | GNOME_VFS_XFER_EMPTY_DIRECTORIES,
                               GNOME_VFS_XFER_ERROR_MODE_QUERY,
                               GNOME_VFS_XFER_OVERWRITE_MODE_QUERY,
                               (GnomeVFSXferProgressCallback) transfer_callback,
                               NULL);
  if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_INTERRUPTED))
    {
      error = g_strdup_printf ("Error: %s", gnome_vfs_result_to_string(result));
      gpe_error_box (error);
      g_free (error);
    }
  else
    {
      gpe_popup_infoprint(GDK_DISPLAY(), _("File copied."));
    }
  gnome_vfs_uri_unref(uri_src);
  gnome_vfs_uri_unref(uri_dest);
}

static void
copy_one_file (gpointer file, gpointer userdata)
{
  gchar *target_file;
  target_file = 
    g_strdup_printf("%s/%s",
                    current_directory,
                    g_path_get_basename(file));
  if (strcmp(file, target_file)) /* check if not the same */
    copy_file(file, target_file);
  g_free(target_file);
}

void 
paste_file_clip (void)
{
  if (file_clipboard == NULL) 
    return;
  
  g_list_foreach(file_clipboard, copy_one_file, NULL);
  
  refresh_current_directory(); 
}

gboolean 
delete_cb_recursive(const gchar *rel_path,
                   GnomeVFSFileInfo *info,
                   gboolean recursing_will_loop,
                   gpointer data,
                   gboolean *recurse)
{
   gchar *path = data;
   gchar *fn = g_strdup_printf("%s/%s", path, rel_path);
  
   if (info->type == GNOME_VFS_FILE_TYPE_DIRECTORY)
   {
     *recurse = TRUE;
     GnomeVFSResult r = 0;
     if (!recursing_will_loop)
     r = gnome_vfs_directory_visit (fn,
                                GNOME_VFS_FILE_INFO_DEFAULT,
                                GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
                                (GnomeVFSDirectoryVisitFunc) delete_cb_recursive,
                                strdup(fn)); 
     /* HACK: We collect the empty directories in a list to delete after 
              visiting the complete tree to prevent gnome-vfs from stopping
              visiting directories 
     */
     if (r == GNOME_VFS_OK)
     {
       dirlist = g_list_append(dirlist, fn);
     }
     else
       fprintf(stderr, "Err %s\n", gnome_vfs_result_to_string(r));

   }
   else
   {
     gnome_vfs_unlink_from_uri (gnome_vfs_uri_new (fn));
     g_free(fn);
   }
   
   return TRUE;
}


static GnomeVFSResult
delete_directory_recursive(FileInformation *dir)
{
  gchar *path = g_strdup(dir->filename);
  GList *iter;
  gnome_vfs_directory_visit    (dir->filename,
                                GNOME_VFS_FILE_INFO_DEFAULT,
                                GNOME_VFS_DIRECTORY_VISIT_DEFAULT,
                                (GnomeVFSDirectoryVisitFunc) delete_cb_recursive,
                                path); 
  g_free(path);
  iter = dirlist;
  while (iter)
    {
      gnome_vfs_remove_directory(iter->data);
      g_free(iter->data);
      iter=iter->next;
    }
  g_list_free(dirlist);
  dirlist = NULL;    
  return gnome_vfs_remove_directory_from_uri(gnome_vfs_uri_new (dir->filename));
}


static void
delete_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                 gpointer data)
{
  gboolean isdir;
  GnomeVFSURI *uri;
  GnomeVFSResult r;
  gint col = (gint)data;

  FileInformation *cfi = NULL;
  
  gtk_tree_model_get(model, iter, col, &cfi, -1);
  if (cfi)
  {
    isdir = (cfi->vfs->type == GNOME_VFS_FILE_TYPE_DIRECTORY);
      if (isdir)
        r = delete_directory_recursive(cfi);
      else
        {
          uri = gnome_vfs_uri_new (cfi->filename);
          r = gnome_vfs_unlink_from_uri (uri);
          gnome_vfs_uri_unref(uri);
        }

      if (r != GNOME_VFS_OK)
        gpe_error_box (gnome_vfs_result_to_string (r));
  }
}


void
ask_delete_file (GtkWidget *active_view, gint col)
{
  GtkTreeSelection *sel = NULL;
  
  if (active_view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  
  if (!sel) 
    return;
  
  if (gtk_tree_selection_count_selected_rows(sel))
  {
    if (gpe_question_ask (_("Delete selected object(s)?"), 
                          _("Confirm"), "!gtk-dialog-question", 
	                     "!gtk-cancel", NULL, "!gtk-delete", NULL, 
                         NULL, NULL) == 1)
      {
        gtk_tree_selection_selected_foreach(sel, 
                                            (GtkTreeSelectionForeachFunc)delete_one_file,
                                            (gpointer)col);
        refresh_current_directory();
      }
  }
}


static void
move_one_file (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
               gpointer data)
{
  GnomeVFSResult result;
  t_movedata *md = (t_movedata*)data;
  gchar *dest;

  FileInformation *cfi = NULL;
  
  gtk_tree_model_get(model, iter, md->col, &cfi, -1);
  if (cfi)
    {
      dest = g_strdup_printf ("%s/%s", md->destination, cfi->vfs->name);
      result = gnome_vfs_move_uri (gnome_vfs_uri_new (cfi->filename), 
                                   gnome_vfs_uri_new (dest), TRUE);
      g_free (dest); 
      if (result != GNOME_VFS_OK)
        gpe_error_box (gnome_vfs_result_to_string (result));
    }
}

static void
move_files (gchar *directory)
{
  GtkTreeSelection *sel = NULL;
   
  if (md.view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(md.view));
  
  if (!sel) 
    return;
  
  md.destination = directory;
  
  if (gtk_tree_selection_count_selected_rows(sel))
  {
    gtk_tree_selection_selected_foreach(sel, 
                                        (GtkTreeSelectionForeachFunc)move_one_file,
                                        (gpointer)&md);
    refresh_current_directory();
    gpe_popup_infoprint(GDK_DISPLAY(), _("File(s) moved."));
  }
}

void
ask_move_file (GtkWidget *active_view, gint col)
{
  GtkWidget *dirbrowser_window;
  GtkTreeSelection *sel = NULL;

  if (active_view)
    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(active_view));
  
  if (!sel) 
    return;

  md.col = col;
  md.view = active_view;
  
  if (gtk_tree_selection_count_selected_rows(sel))
  {
    dirbrowser_window = gpe_create_dir_browser (_("Move to directory..."), 
                                                (gchar *) g_get_home_dir (), 
                                                GTK_SELECTION_SINGLE, move_files);
    gtk_window_set_transient_for (GTK_WINDOW (dirbrowser_window), GTK_WINDOW (window));

    gtk_widget_show_all (dirbrowser_window);
  }
}
