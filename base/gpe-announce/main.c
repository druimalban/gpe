#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "announce.h"

#define _(_x) gettext (_x)

struct gpe_icon my_icons[] = {
  { "bell", "bell" },
  {NULL, NULL}
};

int times=0;

int
main (int argc, char *argv[])
{
  char *announcetext;
  GtkWidget *window;

  g_thread_init(NULL);

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (argc<2) {
    announcetext = (char *)malloc(24+1);
    strcpy(announcetext, "You have an appointment!");
  }
  else announcetext=argv[1];
  
  gtk_init (&argc, &argv);
  
  window = create_window (announcetext);
  gtk_widget_show (window);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_main ();
  return 0;
}

