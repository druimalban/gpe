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
 

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "interface.h"
#include "support.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

GtkWidget *GPE_DB_Main;
GtkWidget *DBFileSelector;

int
main (int argc, char *argv[])
{
gchar initpath[PATH_MAX];
GtkWidget *widget;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  GPE_DB_Main = create_GPE_DB_Main ();
  widget=lookup_widget(GPE_DB_Main, "MainMenu");
  gtk_widget_hide(widget);

  DBFileSelector = create_DBSelection ();
  if (getenv("HOME") != NULL) {
  	initpath[0]='\0';
  	strcpy(initpath, getenv("HOME"));
  	strcat(initpath,"/.gpe/");
  	gtk_file_selection_set_filename(GTK_FILE_SELECTION(DBFileSelector), initpath);
  }

  gtk_widget_show (GPE_DB_Main);

  gtk_main ();
  return 0;
}

