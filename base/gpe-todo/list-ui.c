/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include "todo.h"
#include "pixmaps.h"
#include "errorbox.h"
#include "render.h"

#include "tick.xpm"
#include "box.xpm"

#define _(_x) gettext(_x)

static GdkPixmap *tick_pixmap, *box_pixmap;
static GdkBitmap *tick_bitmap, *box_bitmap;

static guint ystep;
static guint xcol = 18;

GtkWidget *g_draw;

static GtkWidget *g_option;

static struct todo_list *curr_list;

static void lists_menu (void);

static void
set_cur_list(GtkWidget *w, gpointer user_data)
{
  curr_list = user_data;
  gtk_widget_draw (g_draw, NULL);
}

static void
new_todo_item(GtkWidget *w, gpointer user_data)
{
  if (curr_list)
    {
      GtkWidget *todo = edit_todo (curr_list, NULL);
      gtk_widget_show_all (todo);
    }
}

static void
ui_del_list (GtkWidget *widget,
	  gpointer user_data)
{
  GtkCList *clist = GTK_CLIST (user_data);
  if (clist->selection)
    {
      guint row = (guint)clist->selection->data;
      struct todo_list *t = gtk_clist_get_row_data (clist, row);
      del_list (t);
      gtk_clist_remove (clist, row);
      gtk_widget_draw (GTK_WIDGET (clist), NULL);
      lists_menu ();
    }
}

static void
ui_create_new_list(GtkWidget *widget,
		   GtkWidget *d)
{
  GtkWidget *entry = gtk_object_get_data (GTK_OBJECT (d), "entry");
  char *title = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  int id;
  GSList *l;
  GtkWidget *clist = gtk_object_get_data (GTK_OBJECT (d), "clist");
  gchar *line_info[1];
  guint row;
  struct todo_list *t;

  if (title[0] == 0)
    {
      gpe_error_box (_("List title must not be blank"));
      gtk_widget_destroy (d);
      return;
    }
  
  for (l = lists; l; l = l->next)
    {
      struct todo_list *t = l->data;
      if (!strcmp (title, t->title))
	{
	  gpe_error_box (_("A list by that name already exists"));
	  gtk_widget_destroy (d);
	  return;
	}
    }

  id = new_list_id ();
  t = new_list (id, title);
  sql_add_list (id, title);
  line_info[0] = title;
  row = gtk_clist_append (GTK_CLIST (clist), line_info);
  gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
  lists_menu ();
  gtk_widget_destroy (d);
}

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

static void
new_list_box (GtkWidget *w, gpointer data)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_DIALOG);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *ok;
  GtkWidget *cancel;
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *label = gtk_label_new ("Name:");
  GtkWidget *name = gtk_entry_new ();
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

  ok = gpe_picture_button (window->style, _("OK"), "ok");
  cancel = gpe_picture_button (window->style, _("Cancel"), "cancel");

  gtk_widget_show (ok);
  gtk_widget_show (cancel);
  gtk_widget_show (buttons);
  gtk_widget_show (label);
  gtk_widget_show (hbox);
  gtk_widget_show (name);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), name, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttons), ok, FALSE, FALSE, 2);
  gtk_box_pack_end (GTK_BOX (buttons), cancel, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);

  gtk_object_set_data (GTK_OBJECT (window), "entry", name);
  gtk_object_set_data (GTK_OBJECT (window), "clist", data);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked",
		      GTK_SIGNAL_FUNC (ui_create_new_list), window);
  
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
		      close_window, window);

  gtk_widget_show (vbox);

  gtk_window_set_title (GTK_WINDOW (window), _("New list"));

  gtk_container_add (GTK_CONTAINER (window), vbox);
  gtk_widget_show (window);
  gtk_widget_grab_focus (name);
}

static void
close_configure (GtkWidget *w, gpointer data)
{
  gtk_widget_destroy (data);
}

static void
configure(GtkWidget *w, gpointer list)
{
  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *clist = gtk_clist_new (1);
  GtkWidget *pw;
  GSList *l;

  for (l = lists; l; l = l->next)
    {
      struct todo_list *t = l->data;
      gchar *line_info[1];
      guint row;
      
      line_info[0] = t->title;
      row = gtk_clist_append (GTK_CLIST (clist), line_info);
      gtk_clist_set_row_data (GTK_CLIST (clist), row, t);
    }
  
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);

  gtk_widget_realize (window);

  pw = gpe_render_icon (window->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("New"), 
			   _("Create a new list"), 
			   _("Create a new list"),
			   pw, new_list_box, clist);

  pw = gpe_render_icon (window->style, gpe_find_icon ("delete"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Delete"),
			   _("Delete the selected list"), 
			   _("Delete the selected list"),
			   pw, ui_del_list, clist);

  pw = gpe_render_icon (window->style, gpe_find_icon ("cancel"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Close"),
			   _("Close this window"), 
			   _("Close this window"),
			   pw, close_configure, window);

  gtk_widget_show (toolbar);
  gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

  gtk_widget_show (clist);
  gtk_box_pack_start (GTK_BOX (vbox), clist, TRUE, TRUE, 0);

  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  
  gtk_widget_set_usize (window, 240, 320);

  gtk_widget_show (window);
}

static void
purge_completed(GtkWidget *w, gpointer list)
{
  GList *iter = curr_list->items;
  
  while (iter)
    {
      struct todo_item *i = iter->data;
      GList *new_iter = iter->next;
      if (i->state == COMPLETED)
	delete_item (curr_list, i);

      iter = new_iter;
    }

  gtk_widget_draw (g_draw, NULL);
}

static void
show_hide_completed(GtkWidget *w, gpointer list)
{
  if (hide) hide=0;
  else hide=1;
  gtk_widget_draw (g_draw, NULL);
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEvent  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *black_gc;
  GdkGC *gray_gc;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  guint y;
  guint skew = 2;
  GList *iter;
  GdkFont *font = widget->style->font;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  gray_gc = widget->style->bg_gc[GTK_STATE_NORMAL];
  black_gc = widget->style->black_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  ystep = font->ascent + font->descent;
  if (ystep < 14)
    ystep = 14;

  if (! tick_pixmap)
    {
      tick_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						  &tick_bitmap,
						  NULL,
						  tick_xpm);
      box_pixmap = gdk_pixmap_create_from_xpm_d (widget->window,
						 &box_bitmap,
						 NULL,
						 box_xpm);
    }
  
  gdk_draw_rectangle(drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  y = skew;

  if (curr_list)
    {
      for (iter = curr_list->items; iter; iter = iter->next)
	{
	  struct todo_item *i = iter->data;
	  
	  if (i->state != COMPLETED)
	    {
	      gdk_draw_text (drawable, font, black_gc, xcol, y + font->ascent, 
			 i->summary, strlen (i->summary));
	      gdk_draw_pixmap (drawable, black_gc, box_pixmap,
			       2, 0, 2, y - skew, 14, 14);
	      i->pos=y/ystep;
	      y += ystep;
	    }
	  
	}
    
      if (!hide) 
      {
        for (iter = curr_list->items; iter; iter = iter->next)
	  {
	   struct todo_item *i = iter->data;
	   
	   if (i->state == COMPLETED)
	     {
	       gdk_draw_text (drawable, font, black_gc, xcol, y + font->ascent, 
	  		  i->summary, strlen (i->summary));
	   
	       gdk_draw_line (drawable, black_gc, xcol, 
	  		      y + (font->ascent + font->descent) / 2, 
	  		      18 + gdk_string_width (font, i->summary), 
	  		      y + (font->ascent + font->descent) / 2);
	       gdk_draw_pixmap (drawable, black_gc, tick_pixmap,
	  			2, 0, 2, y - skew, 14, 14);
	       i->pos=y/ystep;
	       y += ystep;
	     }
           }
	}
    }
  return TRUE;
}

static void
draw_click_event (GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer user_data)
{
  unsigned int idx = event->y/ystep;

  if (curr_list)
    {
      if (event->type == GDK_BUTTON_PRESS && event->x < xcol)
	{
	  GList *iter=NULL;
  	  for (iter = curr_list->items; iter; iter = iter->next)
	    {
	      struct todo_item *ti = iter->data;
	      if (idx == ti->pos)
	      {
	        if (ti->state == COMPLETED)
	          ti->state = NOT_STARTED;
	        else
	          ti->state = COMPLETED;
	        
	        push_item (ti);
	        gtk_widget_draw (g_draw, NULL);
		break;
	      }
            }
	}
      else if (event->type == GDK_2BUTTON_PRESS)
	{
	  GList *iter=NULL;
  	  for (iter = curr_list->items; iter; iter = iter->next)
	   {
	     struct todo_item *ti = iter->data;
	     if (idx == ti->pos)
	      {
	        gtk_widget_show_all (edit_todo (curr_list, ti));
	        break;
              }
           }
	}
    }
}

static void
lists_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GSList *l;

  for (l = lists; l; l = l->next)
    {
      struct todo_list *t = l->data;
      GtkWidget *i = gtk_menu_item_new_with_label (t->title);
      gtk_menu_append (GTK_MENU (menu), i);
      gtk_signal_connect (GTK_OBJECT (i), "activate", set_cur_list, t);
      gtk_widget_show (i);
    }

  gtk_widget_show (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (g_option), menu);
}

GtkWidget *
top_level (GtkWidget *window)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *sep = gtk_vseparator_new ();
  GtkWidget *toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  GtkWidget *option = gtk_option_menu_new ();
  GtkWidget *pw;
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  
  g_option = option;
  lists_menu ();

  if (lists)
    curr_list = lists->data;

  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);

  pw = gpe_render_icon (window->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("New"), 
			   _("Add a new item"), 
			   _("Add a new item"),
			   pw, new_todo_item, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Configure"), 
			   _("Configure lists"), 
			   _("Configure lists"),
			   pw, configure, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("clean"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Purge completed"), 
			   _("Purge completed"), 
			   _("Purge completed"),
			   pw, purge_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("hide"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Show/Hide completed"), 
			   _("Show/Hide completed"), 
			   _("Show/Hide completed"),
			   pw, show_hide_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("exit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), 
			   _("Exit"), 
			   _("Exit"), 
			   _("Exit"),
			   pw, gtk_exit, NULL);

  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (sep);
  gtk_widget_show (option);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), option, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (draw), "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (draw_click_event), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  gtk_widget_show (draw);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					 draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (vbox);

  g_draw = draw;

  return vbox;
}
