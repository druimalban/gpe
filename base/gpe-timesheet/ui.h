#ifndef _GPE_TIMESHEET_UI_H
  #define _GPE_TIMESHEET_UI_H
#endif

#define MY_PIXMAPS_DIR PREFIX "/share/pixmaps/gpe/default"
#ifdef IS_HILDON
  #define ICON_PATH PREFIX "/share/icons/hicolor/26x26/hildon"
#endif

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>

// hildon includes
#ifdef IS_HILDON
#if HILDON_VER > 0
  #include <hildon/hildon-program.h>
  #include <hildon/hildon-window.h>
#else
  #include <hildon-widgets/hildon-app.h>
  #include <hildon-widgets/hildon-appview.h>
#endif /* HILDON_VER */
  #include <libosso.h>
  #define OSSO_SERVICE_NAME "gpe_timesheet"
#endif /* IS_HILDON */

// gdk-gtk-gpe includes
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>
#include <gpe/gpehelp.h>
#include <gpe/gpedialog.h>

// timesheet includes
#include "sql.h"
#include "journal.h"

/* common variables */
GtkTreeStore *global_task_store;

/* function definitions */
static void show_help (void);
static void view_selected_row_cb (GtkTreeSelection *selection, gpointer data);
static void ui_new_task (GtkWidget *w, gpointer p);
static void ui_delete_task (GtkWidget *w, gpointer data);
static void start_timing (GtkWidget *w, gpointer data);
static void stop_timing (GtkWidget *w, gpointer data);
void prepare_onscreen_journal (GtkTreeSelection *selection, gpointer data);
static void toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data);
GtkWidget * create_interface(GtkWidget *main_window);

