/*
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"
#include "support.h"

int
main (int argc, char *argv[])
{
  GtkWidget *mainw;
  GtkWidget *detailview;
  GtkWidget *edit;
  GtkWidget *calender;
  GtkWidget *fileselection1;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  mainw = create_main ();
  gtk_widget_show (mainw);

#if 0
  detailview = create_detailview ();
  /* gtk_widget_show (detailview); */
  edit = create_edit ();
  /* gtk_widget_show (edit); */
  calender = create_calender ();
  /* gtk_widget_show (calender); */
  fileselection1 = create_fileselection1 ();
  /* gtk_widget_show (fileselection1); */
#endif

  gtk_main ();
  return 0;
}

