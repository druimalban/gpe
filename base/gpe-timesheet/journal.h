#ifndef _GPE_TIMESHEET_JOURNAL_H
#define _GPE_TIMESHEET_JOURNAL_H

#include <gtk/gtk.h>

void journal_list_header(char* title);
void journal_list_add_line(time_t tstart, time_t tstop, 
                     const char *istart, const char *istop);
void journal_list_footer();

extern GtkListStore *journal_list;

typedef enum
{
  JL_ID,
  JL_DURATION,
  JL_INT_START,
  JL_TIME_START,
  JL_START_NOTES,
  JL_INT_END,
  JL_TIME_END,
  JL_END_NOTES,
  JL_NUM_COLS
} journal_list_columns;

#endif

