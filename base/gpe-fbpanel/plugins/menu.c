/*
 * gpe-fbpanel
 *
 * A panel for GPE based on fbpanel
 * 
 * (c) Florian Boor <fb@kernelconcepts.de> 2006
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 *  Application launcher menu.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gpe/spacing.h>
#include <gpe/infoprint.h>
#include <gpe/launch.h>
#include <gpe/desktop_file.h>
#include <gpe/pixmaps.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"
#include "bg.h"
#include "gtkbgbox.h"

#include "dbg.h"

static  gint MENU_ICON_SIZE = 16;
#define GPE_VFOLDERS PREFIX "/share/gpe/vfolders"
#define MATCHBOX_VFOLDERS PREFIX "/usr/share/matchbox/vfolders"

#define _(x) gettext(x)

static GList *folderlist = NULL;

typedef struct {
    GtkTooltips *tips;
    GtkWidget *menu, *box, *bg, *label;
    gulong handler_id;
    gint iconsize, paneliconsize;
    GSList *files;
} menup;

/* Data type for application folder/tab encapsulation. */
typedef struct {
    gchar *name;          /* name / label */
    gchar *iconname;      /* icon filename */
    gchar *category;      /* matching category, maybe wants to be extended to multiple */
    GtkWidget *icon;      /* icon widget */
    GtkWidget *menu;      /* associated submenu */
    GSList *applications;
} t_folder;

static t_folder *default_folder = NULL;
static t_folder *panel_folder = NULL;

typedef struct {
    gchar *name;          /* name / label */
    gchar *iconname;      /* icon filename */
    gchar *exec;          /* executable */
    GtkWidget *icon;      /* icon widget */
    gboolean needs_terminal;
    gboolean startup_notify;
    gboolean single_instance;
} t_application;

static GList *actions = NULL;

/* Callback used to track the startup status of applications. */
static void
launch_status_callback (enum launch_status status, void *data)
{
   
  switch (status)
    {
    case LAUNCH_STARTING:
      {
    	break;
      }

    case LAUNCH_COMPLETE:
    case LAUNCH_FAILED:
      {
      }
      break;
    }
}


/* Start an application honoring single instance applications and notification. */
static void
run_application (GtkWidget *widget, gpointer user_data)
{
  t_application *app = (t_application*)user_data;
  gchar *s;
  Window w;
  gboolean single_instance = app->single_instance;
  gboolean startup_notify = app->startup_notify;
  gchar *text;
  Display *dpy = GDK_DISPLAY();
    
  if (app->name == NULL) 
    app->name = g_strdup (app->exec);

  text = g_strdup_printf ("%s %s", _("Starting"), app->name);
  gpe_popup_infoprint (dpy, text);
  g_free (text);

  /* Can't have single instance without startup notification */
  if (!startup_notify)
    single_instance = FALSE;

  /* i guess we can do that safely */
  s = strstr (app->exec, "%U");
  if (!s)
    s = strstr (app->exec, "%f");
  if (s)
    *s = 0;
  
  if (!single_instance)
    {
      gpe_launch_program_with_callback (dpy, app->exec, app->name, startup_notify, 
					launch_status_callback, app);
      return;
    }

  w = gpe_launch_get_window_for_binary (dpy, app->exec);
  if (w)
    {
      gpe_launch_activate_window (dpy, w);
    }
  else
    {
      if (! gpe_launch_startup_is_pending (dpy, app->exec))
        {
          /* Actually run the program */
          gpe_launch_program_with_callback (dpy, app->exec, app->name, TRUE,
                                            launch_status_callback, app);
        }
    }
}


/* Create a new application folder widget from its desktop description. */
static t_folder*
new_folder_from_desktop (GnomeDesktopFile *g)
{
  t_folder *result = g_malloc0 (sizeof(t_folder));
  gchar *str;
  GdkPixbuf *pbuf, *iconbuf;
  
  gnome_desktop_file_get_string (g, NULL, "Name", &str);
  if (!str)
    {
      g_free (result);
      return NULL;
    }
  result->name = str;
  gnome_desktop_file_get_string (g, NULL, "Match", &str);
  result->category = str;
  if (!str)
    {
        g_free (result->name);
        g_free (result);
        return NULL;        
    }
  gnome_desktop_file_get_string (g, NULL, "Icon", &str);
  result->iconname = str;
  
  str = g_strdup_printf ("%s/%s", GPE_PIXMAPS_DIR, result->iconname);
  pbuf = gdk_pixbuf_new_from_file (str, NULL);
  if (pbuf)
    {
      iconbuf = gdk_pixbuf_scale_simple (pbuf, MENU_ICON_SIZE, 
                                         MENU_ICON_SIZE, GDK_INTERP_BILINEAR);
      result->icon = gtk_image_new_from_pixbuf (iconbuf);
    
      g_object_unref (pbuf);
      g_object_unref (iconbuf);
    }
  if (str)
    g_free (str);
 
  /* is this our default folder? */
  if (!strcmp("Other", result->category))
      default_folder = result;
    
  return result;
}

/* Create a new allpication data set from its desktop description and add this
 * to the list. */
t_application *
new_application_from_desktop (GnomeDesktopFile *g)
{
  gchar *name, *icon, *exec;
  gboolean terminal;
  gboolean startup;
  gboolean single = TRUE;
  gboolean bv;
  GdkPixbuf *ibuf;
  t_application *app = g_malloc0 (sizeof(t_application));
  
    
  gnome_desktop_file_get_string (g, NULL, "Name", &name);
  gnome_desktop_file_get_string (g, NULL, "Icon", &icon);
  gnome_desktop_file_get_string (g, NULL, "Exec", &exec);
  if (gnome_desktop_file_get_boolean (g, NULL, "Terminal", &terminal) == FALSE)
      terminal = FALSE;
  if (gnome_desktop_file_get_boolean (g, NULL, "StartupNotify", &startup) == FALSE)
      startup = TRUE;
  if (gnome_desktop_file_get_boolean (g, NULL, "SingleInstance", &bv) && (bv == FALSE))
      single = FALSE;
  if (gnome_desktop_file_get_boolean (g, NULL, "X-SingleInstance", &bv) && (bv == FALSE))
      single = FALSE;

  ibuf = load_icon_scaled (icon, MENU_ICON_SIZE);

  app->name = name;
  app->iconname = icon;
  app->exec = exec;
  app->needs_terminal = terminal;
  app->startup_notify = startup;
  app->single_instance = single;
  
  if (ibuf) 
    {
      app->icon = gtk_image_new_from_pixbuf (ibuf);
      g_object_unref (ibuf);
    }
    
  return app;
}

/* Load all available desktop descriptions of folders. */
static GList*
load_folder_list (const gchar *path)
{
  DIR *dir;
  struct dirent *entry;
  GList *result = NULL;
  t_folder *new_folder;

  dir = opendir (path);
  if (dir)
    {
      while ((entry = readdir (dir)))
      {
	  char *temp;
	  GnomeDesktopFile *p;
	  GError *err = NULL;
	  
	  if (entry->d_name[0] == '.')
	    continue;
	
      temp = g_strdup_printf ("%s/%s", path, entry->d_name);
	  p = gnome_desktop_file_load (temp, &err);
	  if (p)
	    {
	      gchar *type;
	      gnome_desktop_file_get_string (p, NULL, "Type", &type);

	      if (type == NULL || strcmp (type, "Directory"))
              gnome_desktop_file_free (p);
	      else
            {
              new_folder = new_folder_from_desktop (p);
              if (new_folder)
                result = g_list_append (result, new_folder);
              gnome_desktop_file_free (p);
            }

	      if (type)
             g_free (type);
        }
	  else
	    fprintf (stderr, "Couldn't load \"%s\": %s\n", temp, err->message);

	  if (err)
	    g_error_free (err);

	  g_free (temp);
    }
    closedir (dir);
  }
  return result;
}


/* Add a new application description to a folder */
static void
add_application (const gchar *filename)
{
  GnomeDesktopFile *p;
  GError *err = NULL;
    
  p = gnome_desktop_file_load (filename, &err);
  if (p)
  {
      gchar *type;
      t_application *app;
      GtkWidget *item;
      
      gnome_desktop_file_get_string (p, NULL, "Type", &type);

      if (type == NULL)
        {
          gnome_desktop_file_free (p);
        }
      else 
      if (!strcmp (type, "Application"))
        {
          GList *iter;
          gchar *cat;              
          
          gnome_desktop_file_get_string (p, NULL, "Categories", &cat);
          for (iter = folderlist; iter; iter = iter->next)
            {
              t_folder *folder = iter->data;
              if (cat && strstr(cat, folder->category))
                {
                  app = new_application_from_desktop (p);
                  folder->applications = g_slist_prepend (folder->applications, app);
                  item = gtk_image_menu_item_new_with_label (app->name);
                  g_signal_connect (G_OBJECT(item), "activate", 
                                    G_CALLBACK(run_application), (gpointer)app);
                  if (app->icon)
                      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), app->icon);
                  gtk_menu_append (GTK_MENU_SHELL(folder->menu), item);
                  break;
                }
            }
          if ((iter == NULL) && (default_folder != NULL)) /* no folder found */
            {
                app = new_application_from_desktop (p);
                default_folder->applications = 
                      g_slist_prepend (default_folder->applications, app);
                item = gtk_image_menu_item_new_with_label (app->name);
                g_signal_connect (G_OBJECT(item), "activate", 
                                  G_CALLBACK(run_application), (gpointer)app);
                if (app->icon)
                    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), app->icon);
                gtk_menu_append (GTK_MENU(default_folder->menu), item);
            }
          if (strstr (cat, "Action"))
            {
                app = new_application_from_desktop (p);
                actions = g_list_append (actions, app);
            }
                  
          g_free (cat);
          gnome_desktop_file_free (p);
       }
     else 
     if (!strcmp (type, "PanelApp"))
       {
          if (!panel_folder->menu)
            {
              panel_folder->menu = gtk_menu_new();
            }
          app = new_application_from_desktop (p);
          panel_folder->applications = g_slist_prepend (panel_folder->applications, app);
          item = gtk_image_menu_item_new_with_label (app->name);
          g_signal_connect (G_OBJECT(item), "activate", 
                            G_CALLBACK(run_application), (gpointer)app);
          if (app->icon)
             gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), app->icon);
          gtk_menu_append (GTK_MENU_SHELL(panel_folder->menu), item);
       }
       
       if (type)
           g_free (type);
  }
  else
     fprintf (stderr, "Couldn't load \"%s\": %s\n", filename, err->message);

  if (err)
     g_error_free (err);
}

/* Load all available application descriptions. */
static void
load_application_list (const gchar *path)
{
  DIR *dir;
  struct dirent *entry;

  dir = opendir (path);
  if (dir)
  {
      while ((entry = readdir (dir)))
      {
        char *temp;	  
	  
        if (entry->d_name[0] == '.')
          continue;
	
        temp = g_strdup_printf ("%s/%s", path, entry->d_name);
        add_application (temp);
        g_free (temp);
      }
    closedir (dir);
  }
}


static void
menu_destructor(plugin *p)
{
    menup *m = (menup *)p->priv;

    ENTER;
    g_signal_handler_disconnect(G_OBJECT(m->bg), m->handler_id);
    gtk_widget_destroy(m->menu);
    gtk_widget_destroy(m->box);
    g_free(m);
    RET();
}

static void
menu_pos(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *widget)
{
    int ox, oy, w, h;
    plugin *p;
    
    ENTER;
    p = g_object_get_data(G_OBJECT(widget), "plugin");
    gdk_window_get_origin(widget->window, &ox, &oy);
    w = GTK_WIDGET(menu)->requisition.width;
    h = GTK_WIDGET(menu)->requisition.height;
    if (p->panel->orientation == ORIENT_HORIZ) {
        *x = ox;
        if (*x + w > gdk_screen_width())
            *x = ox + widget->allocation.width - w;
        *y = oy - h;
        if (*y < 0)
            *y = oy + widget->allocation.height;
    } else {
        *x = ox + widget->allocation.width;
        if (*x > gdk_screen_width())
            *x = ox - w;
        *y = oy;
        if (*y + h >  gdk_screen_height())
            *y = oy + widget->allocation.height - h;        
    }
    DBG("widget: x,y=%d,%d  w,h=%d,%d\n", ox, oy,
          widget->allocation.width, widget->allocation.height );
    DBG("w-h %d %d\n", w, h);
    *push_in = TRUE;
    RET();
}

static gboolean
my_button_pressed(GtkWidget *widget, GdkEventButton *event, GtkMenu *menu)
{
    ENTER;  
    if ((event->type == GDK_BUTTON_PRESS)
          && (event->x >=0 && event->x < widget->allocation.width)
          && (event->y >=0 && event->y < widget->allocation.height)) {
        gtk_menu_popup(menu,
              NULL, NULL, (GtkMenuPositionFunc)menu_pos, widget, 
              event->button, event->time);
    }
    RET(TRUE);
}


static GtkWidget *
make_button(plugin *p, gchar *fname, gchar *name, GtkWidget *menu)
{
    int w, h;
    menup *m;
    
    ENTER;
    m = (menup *)p->priv;
    m->menu = menu;
    if (p->panel->orientation == ORIENT_HORIZ) {
        w = 10000;
        h = p->panel->ah;
    } else {
        w = p->panel->aw;
        h = 10000;
    }
    m->bg = fb_button_new_from_file_with_label(fname, w, h, 0xFF0000, TRUE,
          (p->panel->orientation == ORIENT_HORIZ ? name : NULL));
    gtk_widget_show(m->bg);  
    gtk_box_pack_start(GTK_BOX(m->box), m->bg, FALSE, FALSE, 0);
    if (p->panel->transparent) 
        gtk_bgbox_set_background(m->bg, BG_ROOT, p->panel->tintcolor, p->panel->alpha);
    

    m->handler_id = g_signal_connect (G_OBJECT (m->bg), "button-press-event",
          G_CALLBACK (my_button_pressed), menu);
    g_object_set_data(G_OBJECT(m->bg), "plugin", p);
   
    RET(m->bg);
}
   
static void
new_application_menu (GtkWidget *main_menu, t_folder *folder)
{
    GtkWidget *item;

    folder->menu = gtk_menu_new();
    
    item = gtk_image_menu_item_new_with_label (folder->name);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), folder->icon);
    gtk_menu_append (main_menu, item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM(item), folder->menu);
}

void
build_menu(plugin *p)
{
  GList *iter;
  GtkWidget *item;
  GtkWidget *icon;
  menup *m = p->priv;
  const gchar *folders = NULL;
    
  item = gtk_event_box_new();
  gtk_container_set_border_width(GTK_CONTAINER(item), 0);
  
  icon = gtk_image_new_from_pixbuf(
            gpe_find_icon_scaled_free ("gpe-logo", m->paneliconsize, m->paneliconsize));
  gtk_widget_show(icon);
  gtk_container_add (GTK_CONTAINER (item), icon);
  gtk_container_add (GTK_CONTAINER (m->box), item);
    
  m->menu = gtk_menu_new ();
  gtk_menu_attach_to_widget (GTK_MENU (m->menu), item, NULL);
  
  g_signal_connect (G_OBJECT(item), "button-press-event", 
                    G_CALLBACK (my_button_pressed), m->menu);
  g_object_set_data(G_OBJECT(item), "plugin", p);
  
  if (!access (MATCHBOX_VFOLDERS, R_OK))
      folders = MATCHBOX_VFOLDERS;
  else
    if (!access (GPE_VFOLDERS, R_OK))
        folders = GPE_VFOLDERS;
  
  folderlist = load_folder_list (folders);
  panel_folder = g_malloc0 (sizeof(t_folder));
    
  for (iter = folderlist; iter; iter = iter->next)
    {
       t_folder *folder = (t_folder*) iter->data;
        
       new_application_menu (m->menu, folder);
    }
    
  load_application_list (PREFIX "/share/applications");

  for (iter = actions; iter; iter = iter->next)
    {
        t_application *app = iter->data;
        
        item = gtk_image_menu_item_new_with_label (app->name);
        g_signal_connect (G_OBJECT(item), "activate", 
                          G_CALLBACK(run_application), (gpointer)app);
        if (app->icon)
          gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(item), app->icon);
        gtk_menu_append (GTK_MENU(m->menu), item);
    }
    
  gtk_widget_show_all (m->menu);
}


static int
menu_constructor(plugin *p)
{
    menup *m;

    ENTER;
    m = g_new0(menup, 1);
    g_return_val_if_fail(m != NULL, 0);
    p->priv = m;

    if  (p->panel->orientation == ORIENT_HORIZ) 
        m->paneliconsize = p->panel->ah
            - 2* GTK_WIDGET(p->panel->box)->style->ythickness;
    else
        m->paneliconsize = p->panel->aw
            - 2* GTK_WIDGET(p->panel->box)->style->xthickness;
    
    m->iconsize = m->paneliconsize * 2 / 3;
    if (m->iconsize < 16)
        m->iconsize = 16;
    MENU_ICON_SIZE = m->iconsize;

    m->box = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(m->box), 0);
    gtk_container_add(GTK_CONTAINER(p->pwid), m->box);

    build_menu (p);
    
    g_object_set_data (G_OBJECT(p->pwid->parent), "tray-apps-menu", 
                       panel_folder->menu);
    
    RET(1);
}


plugin_class menu_plugin_class = {
    fname: NULL,
    count: 0,

    type : "menu",
    name : "menu",
    version: "2.0",
    description : "Provide Menu",

    constructor : menu_constructor,
    destructor  : menu_destructor,
};
