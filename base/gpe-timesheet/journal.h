#ifndef _GPE_TIMESHEET_JOURNAL_H
#define _GPE_TIMESHEET_JOURNAL_H

#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libintl.h>
#include <unistd.h>
#include <time.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>

#include "sql.h"

void journal_list_header(char* title);
void journal_list_add_line(time_t tstart, time_t tstop, 
                     const char *istart, const char *istop, guint *idx, guint status);
void journal_list_footer();

extern gboolean ui_edit_journal (GtkWidget *w, gpointer user_data);
extern void prepare_onscreen_journal (GtkTreeSelection *selection,
                       gpointer user_data);

extern GtkListStore *journal_list;

typedef enum
{
  JL_ID,
  JL_ICON,
  JL_INFO,
  JL_DESCRIPTION,
  JL_START,
  JL_START_NOTES,
  JL_STOP,
  JL_STOP_NOTES,
  JL_TYPE,
  JL_NUM_COLS
} journal_list_columns;

typedef enum
{
  JL_TYPE_START,
  JL_TYPE_STOP,
  JL_TYPE_LAST,
  JL_TYPE_START_OPEN,
  JL_TYPE_STOP_OPEN,
} journal_list_log_type;

#endif

