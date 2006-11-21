/*
 * gpe-mini-browser v0.20
 *
 * Basic web browser based on gtk-webcore 
 * 
 * All functions related to settings and configuration.
 *
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * Honours : To Satyricon's "King" for positive energy
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>


#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <glib/gkeyfile.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/question.h>
#include <gpe/gpedialog.h>
#include <gpe/picturebutton.h>

#include "gpe-mini-browser.h"

#ifdef HILDON
/* Hildon includes */
#include <hildon-widgets/hildon-app.h>
#include <hildon-widgets/hildon-appview.h>
#include <libosso.h>
#endif /*HILDON*/
//#define DEBUG /* uncomment this if you want debug output*/

char * get_conf_file(void);

/*==============================================*/

void set_proxy(Webi * html, WebiSettings * ks)
{
  const gchar *http_proxy;

  http_proxy = g_getenv ("http_proxy");
  if (!http_proxy)
    http_proxy = g_getenv ("HTTP_PROXY");
  if (!http_proxy)
    {
      http_proxy = "";
    }
  ks->http_proxy = g_strdup (http_proxy);

  webi_set_settings (WEBI (html), ks);
}

/*==============================================*/

void
set_default_settings (Webi * html, WebiSettings * ks)
{
#ifndef HILDON
  ks->default_font_size = 11;
  ks->default_fixed_font_size = 11;
#else
  ks->default_font_size = 14;
  ks->default_fixed_font_size = 14;
#endif
  ks->minimum_font_size = 7;
  ks->serif_font_family = "serif";
  ks->sans_serif_font_family = "sans";
  ks->fixed_font_family = "sans";
  ks->standard_font_family = "sans";
  ks->autoload_images = 1;
  ks->javascript_enabled = 1;

  webi_set_settings (WEBI (html), ks);
}

/*==============================================*/

void
zoom_in (GtkWidget * zoom_in, gpointer * data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *) data;
  set = zoom->settings;
  set->default_font_size++;
  set->default_fixed_font_size++;
  webi_set_settings (WEBI (zoom->html), set);
}

/*==============================================*/

void
zoom_out (GtkWidget * zoom_out, gpointer * data)
{
  struct zoom_data *zoom;
  WebiSettings *set;

  zoom = (struct zoom_data *) data;
  set = zoom->settings;
  set->default_font_size--;
  set->default_fixed_font_size--;
  webi_set_settings (WEBI (zoom->html), set);
}

/*==============================================*/

int
parse_settings_file (Webi *html, WebiSettings * ks)
{
  GKeyFile *settingsfile;
  gboolean test;
  gchar **keys;
  char *filename = NULL;

  filename = get_conf_file();
  if(filename == 0)
    {
      gpe_error_box (_("Could not save settings! Make sure HOME is set in your environment"));
      free (filename);
      return 1;
    }
  settingsfile = g_key_file_new();
  test = g_key_file_load_from_file(settingsfile, filename, G_KEY_FILE_NONE, NULL);
  free(filename);
  if(!test)
      return 1;
  keys = g_key_file_get_keys (settingsfile, "Config", NULL, NULL);
  while (*keys != NULL)
    {
      if(!strcmp(*keys, "Proxy"))
      {
       /* first check if the proxy has been set from the environment, if not use the
	  one from the config file */
       if(!strcmp(ks->http_proxy, ""))
	{
       	 ks->http_proxy = g_key_file_get_string(settingsfile, "Config", *keys, NULL);
	}
       goto loopend;
      }
      if(!strcmp(*keys, "AutoImageLoading"))
      {
       ks->autoload_images = g_key_file_get_integer(settingsfile, "Config", *keys, NULL);
       goto loopend;
      }
      if(!strcmp(*keys, "Javascript"))
      {
       ks->javascript_enabled = g_key_file_get_integer(settingsfile, "Config", *keys, NULL);
       goto loopend;
      }
      if(!strcmp(*keys, "Fontsize"))
      {
        ks->default_font_size = g_key_file_get_integer(settingsfile, "Config", *keys, NULL);
        ks->default_fixed_font_size = g_key_file_get_integer(settingsfile, "Config", *keys, NULL);
      }
loopend:      ++keys;
    } 
  webi_set_settings (WEBI (html), ks);
  return 0;
}

/*==============================================*/

int
write_settings_to_file (WebiSettings * ks)
{
  GKeyFile *settingsfile;
  gchar *output;
  FILE *outfile;
  char *filename = NULL;
 
  filename = get_conf_file();
  if(filename == 0)
    {
      gpe_error_box (_
		     ("Could not save settings! Make sure HOME is set in your environment"));
      free (filename);
      return 1;
    }
  outfile = fopen (filename, "w+");
  if (outfile <= 0)
    {
      gpe_error_box (_("Could not write file!"));
      free (filename);
      return 1;
    }

  settingsfile = g_key_file_new ();
  g_key_file_set_string (settingsfile, "Config", "Proxy", ks->http_proxy);
  g_key_file_set_integer (settingsfile, "Config", "AutoImageLoading",
			  ks->autoload_images);
  g_key_file_set_integer (settingsfile, "Config", "Javascript",
			  ks->javascript_enabled);
  g_key_file_set_integer (settingsfile, "Config", "Fontsize",
			  ks->default_font_size);

  output = g_key_file_to_data (settingsfile, NULL, NULL);
  fprintf (outfile, output);
  fclose (outfile);
  free (filename);
  g_key_file_free (settingsfile);
  return 0;
}

/*==============================================*/

void save_settings(GtkWidget * app, WebiSettings * ks)
{
  write_settings_to_file (ks);
}

/*==============================================*/

void apply_settings(GtkWidget * app, gpointer * data)
{
  struct zoom_data *info;

  info = (struct zoom_data *)data;
  webi_set_settings(info->html, info->settings);
}


/*==============================================*/
  
char * get_conf_file(void)
{
  char *filename = NULL;
  const char *home = getenv ("HOME");

  /* check if we can get the users home dir and create the file */
  if (home != NULL)
  {
    size_t len = strlen (home) + strlen (CONF_NAME);
    filename = malloc (len + 1);
    strncpy (filename, home, len);
    strcat (filename, CONF_NAME);
    return filename;
  }
  else
  {
    free(filename);
    return 0;
  }
}

/*==============================================*/

void
show_configuration_panel(GtkWidget *window, gpointer *data)
{
  GtkWidget *config_window, *content_box_left, *content_box_right, *hbox1, *hbox2, *vbox1;  /* main window */
  GtkWidget *proxy_box, *font_box; /* entry boxes */
  GtkWidget *javascript_select, *img_load_select; /* toggle buttons */
  GtkWidget *proxy_label, *font_label; /*labels */
  GtkWidget *apply, *cancel;
  struct zoom_data *info;

  info = (struct zoom_data *)data;
  config_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_modal (GTK_WINDOW(config_window), TRUE);
  gtk_window_set_title (GTK_WINDOW(config_window), _("Configuration"));
  
  gtk_window_set_type_hint (GTK_WINDOW (config_window),
                            GDK_WINDOW_TYPE_HINT_DIALOG);
  gtk_window_set_decorated (GTK_WINDOW (config_window), TRUE);

  /* content boxes to pack labels and entries */
  content_box_left = gtk_vbox_new (FALSE, 0);
  content_box_right = gtk_vbox_new (FALSE, 0);
  hbox1 = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);
  vbox1 = gtk_vbox_new (FALSE, 0);

  /* content of the left box */
  proxy_label = gtk_label_new("proxy server:");
  font_label = gtk_label_new("fontsize:");

  /* content of the right box */
  proxy_box = gtk_entry_new ();
  gtk_entry_set_text(GTK_ENTRY(proxy_box), info->settings->http_proxy);
  font_box = gtk_entry_new();
  char temp[4];
  snprintf(temp, 4, "%g",info->settings->default_font_size);
  gtk_entry_set_text(GTK_ENTRY(font_box), temp);
   
  /* check buttons for images and javascript */
  javascript_select = gtk_check_button_new_with_label("enable javascript");
  img_load_select = gtk_check_button_new_with_label("load images");
  /* set check buttons according to the current status */
  if(info->settings->javascript_enabled)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(javascript_select), TRUE);
  if(info->settings->autoload_images)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(img_load_select), TRUE);

  /* Cancel and apply buttons */
  apply = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  /* pack the boxes */
  gtk_box_pack_start(GTK_BOX(content_box_left), proxy_label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_box_left), font_label, TRUE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(content_box_right), proxy_box, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(content_box_right), font_box, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(hbox1), content_box_left, FALSE, FALSE, 0); 
  gtk_box_pack_start(GTK_BOX(hbox1), content_box_right, FALSE, FALSE, 0); 
  
  gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(cancel), FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox2), GTK_WIDGET(apply), FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0); 
  gtk_box_pack_start(GTK_BOX(vbox1), javascript_select, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), img_load_select, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox1), hbox2, TRUE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(config_window), GTK_WIDGET(vbox1));
  
  /* connect signals do nothing on destroy as it is considered to be 
     the same as cancelling */
  g_signal_connect (GTK_OBJECT (apply), "clicked",
                    G_CALLBACK (apply_settings), info);
  g_signal_connect (GTK_OBJECT (apply), "clicked",
                    G_CALLBACK (save_settings), info->settings);
  g_signal_connect (GTK_OBJECT (apply), "clicked",
                    G_CALLBACK (destroy_window), config_window);
  g_signal_connect (GTK_OBJECT (cancel), "clicked",
                    G_CALLBACK (destroy_window), config_window);
  g_signal_connect (GTK_OBJECT (javascript_select), "toggled", 
 		    G_CALLBACK (toggle_javascript), info->settings);
  g_signal_connect (GTK_OBJECT (img_load_select), "toggled", 
 		    G_CALLBACK (toggle_images), info->settings);
  g_signal_connect (GTK_OBJECT (font_box), "changed",
		    G_CALLBACK (set_font_size), info->settings);
  g_signal_connect (GTK_OBJECT (proxy_box), "changed",
		    G_CALLBACK (set_proxy_config), info->settings);

  
  gtk_widget_show_all(config_window);
}

/*==============================================*/

void toggle_javascript (GtkWidget * button, WebiSettings * ks)
{
  gboolean value;

  value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  ks->javascript_enabled = value;
}

/*==============================================*/

void toggle_images (GtkWidget * button, WebiSettings * ks)
{
  gboolean value;

  value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
  ks->autoload_images = value;
}

/*==============================================*/

void set_font_size (GtkEntry * entry, WebiSettings * ks)
{
  ks->default_font_size = atoi(gtk_entry_get_text(entry));
}

/*==============================================*/

void set_proxy_config (GtkEntry * entry, WebiSettings * ks)
{
  g_free((gpointer *)ks->http_proxy);
  ks->http_proxy = g_strdup(gtk_entry_get_text(entry));
}
