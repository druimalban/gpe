/*
 * $Id$
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "init.h"

#include "interface.h"
#include "support.h"
#include "db.h"

int
main (int argc, char *argv[])
{
  GtkWidget *mainw;

#ifdef ENABLE_NLS
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
#endif

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (db_open ())
    exit (1);

  add_pixmap_directory (PACKAGE_DATA_DIR "/pixmaps");
  add_pixmap_directory (PACKAGE_SOURCE_DIR "/pixmaps");

  mainw = create_main ();
  gtk_widget_show (mainw);

  load_structure ();
  edit_structure ();

  gtk_main ();
  return 0;
}
