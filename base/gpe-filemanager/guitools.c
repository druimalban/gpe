/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gpe/spacing.h>

#define _(x) gettext(x)

typedef struct
{
  gchar *name;
  gchar *path;
  gboolean active;
  GtkWidget *item;
}
t_storage;

static t_storage *storages = NULL;
static int num_storage = 0;


static void
activate_item(int i)
{
  if ((i < 0) || (i > num_storage-1))
    return;
  
  if (GTK_IS_TOGGLE_BUTTON(storages[i].item))
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(storages[i].item), TRUE);
  else if (GTK_IS_RADIO_MENU_ITEM(storages[i].item))
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(storages[i].item), TRUE);
}

void
set_active_item(char* path)
{
  int i;
  
  if (g_str_has_prefix(path, storages[0].path)) /* are we in $HOME? */
    {  
      activate_item(0);
      return;
    }
  else if (strcmp("/", path))
  {
    for (i = 2; i < num_storage; i++)
      {
        if (g_str_has_prefix(path, storages[i].path))
          {
            activate_item(i);
            return;
          }
      }  
  }
  /* nothing? activate filesystem button per default */
  activate_item(1);
}

static void 
filesystem_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
  t_storage *st = user_data;
    
  if (GTK_IS_TOGGLE_BUTTON(togglebutton))
    {
      if (gtk_toggle_button_get_active(togglebutton))
        browse_directory(st->path);
    }
  else if (GTK_IS_RADIO_MENU_ITEM(togglebutton))
    {
      if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(togglebutton)))
        browse_directory(st->path);
    }
}

void 
add_storage(gchar* name, const gchar *path)
{
  num_storage++;
  storages = realloc(storages, sizeof(t_storage) * num_storage);
  storages[num_storage - 1].name = name;
  storages[num_storage - 1].path = g_strdup(path);
  storages[num_storage - 1].active = FALSE;
}


/* searches for interesting devices in fstab */
void
scan_fstab(void)
{
  FILE *fstab = fopen("/etc/fstab", "r");
  
  if (fstab)
    {
      char buf[255];
      while (fgets(buf, 255, fstab))
        {
          char path[128];
          if (sscanf(buf, "%*s %128s %*s %*s %*d %*d", path))
            { 
              if (strstr(path, "card") || strstr(path, "mmc"))
                add_storage(_("MMC-Card"), path);
              else if (strstr(path, "cf") || strstr(path, "hda"))
                add_storage(_("CF-Card"), path);
              else if (strstr(path, "usb"))
                add_storage(_("USB-Device"), path);
              else if (strstr(path, "cdrom"))
                add_storage(_("CDROM"), path);
            }
        }
        fclose(fstab);
    }
}


GtkWidget *
build_storage_menu(gboolean wide)
{
  GtkWidget *vmenu;
  GtkWidget *item = NULL;
  int i;
  
  /* add defaults */
  
  add_storage(_("My Documents"), g_get_home_dir()); /* if you change this order, change below too */
  add_storage(_("Filesystem"), "/");
  scan_fstab();
  
  if (wide)
    {
      vmenu = gtk_hbox_new(FALSE, gpe_get_boxspacing());
      gtk_container_set_border_width(GTK_CONTAINER(vmenu), gpe_get_border());
      item = gtk_radio_button_new_with_label(NULL, storages[0].name);
      storages[0].item = item;
      gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(item), FALSE);
      gtk_box_pack_start(GTK_BOX(vmenu), item, FALSE, TRUE, 0);
      g_signal_connect_after(G_OBJECT(item), "clicked", G_CALLBACK(filesystem_toggled), &storages[0]);
      
      for (i = 1; i < num_storage; i++)
        {
          item = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(item), storages[i].name);
          storages[i].item = item;
          gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(item), FALSE);
          gtk_box_pack_start(GTK_BOX(vmenu), item, FALSE, TRUE, 0);
          g_signal_connect_after(G_OBJECT(item), "clicked", G_CALLBACK(filesystem_toggled), &storages[i]);
        }
    }
  else
    {
      vmenu = gtk_menu_new();
      item = gtk_radio_menu_item_new_with_label(NULL, storages[0].name);
      storages[0].item = item;
      gtk_menu_append(vmenu, item);
      g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(filesystem_toggled), &storages[0]);
      
      for (i = 1; i < num_storage; i++)
        {
          item = gtk_radio_menu_item_new_with_label_from_widget(GTK_RADIO_MENU_ITEM(item), storages[i].name);
          storages[i].item = item;
          gtk_menu_append(vmenu, item);
          g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(filesystem_toggled), &storages[i]);
        }
    }      
  
  return vmenu;
}
