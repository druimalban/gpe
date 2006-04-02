#ifndef _GPE_TIMESHEET_UI_H
  #define _GPE_TIMESHEET_UI_H
#endif

/* nasty workaround for evil application installer in maemo */
#ifndef IS_HILDON
  #define MY_PREFIX PREFIX
#else
  #define MY_PREFIX "/var/lib/install/" PREFIX 
#endif
/* end of nasty workaround */

#define MY_PIXMAPS_DIR MY_PREFIX "/share/gpe/pixmaps/default"
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
  #include <hildon-widgets/hildon-app.h>
  #include <hildon-widgets/hildon-appview.h>
  #include <libosso.h>
  #define OSSO_SERVICE_NAME "gpe_timesheet"
#endif

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
#include "html.h"
#include "journal.h"

static void show_help (void);
static void view_selected_row_cb (GtkTreeSelection *selection, gpointer data);
static void ui_new_task (GtkWidget *w, gpointer p);
static void ui_delete_task (GtkWidget *w, gpointer data);
static void start_timing (GtkWidget *w, gpointer data);
static void stop_timing (GtkWidget *w, gpointer data);
void prepare_onscreen_journal (GtkTreeSelection *selection, gpointer data);
gboolean stop_timing_all_cb (GtkTreeModel *m, GtkTreePath *p, GtkTreeIter *i, gpointer data);
static void toggle_toolbar(GtkCheckMenuItem *menuitem, gpointer user_data);
static GtkWidget * create_main_toolbar (void);

GtkTreeStore *global_task_store;
GtkTreeIter *taskiter;

