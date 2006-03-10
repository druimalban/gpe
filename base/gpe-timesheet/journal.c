/*
 * This file is part of gpe-timeheet
 * (c) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 * (c) 2005 Philippe De Swert <philippedeswert@scarlet.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gpe/errorbox.h>
#include <libintl.h>
#include <unistd.h>
#include <time.h>

//#include "sql.h"
#include "journal.h"

#define _(x) gettext(x)

static time_t totaltime = 0;

GtkListStore  *journal_list;
GtkTreeIter   iter;

/* adds the document header */
void
journal_list_header(char* title)
{
  gtk_list_store_clear (journal_list);
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set (journal_list, &iter,
    JL_DURATION, _("Duration"),
    JL_TIME_START, _("Start"),
    JL_START_NOTES, _("Notes"),
    JL_TIME_END, _("End"),
    JL_END_NOTES, _("Notes"),
    -1);
  totaltime = 0;
  return;
}

/* adding data lines to journal listview */
void
journal_list_add_line(time_t tstart, time_t tstop, 
                 const char *istart, const char *istop)
{
  gchar *starttm, *stoptm;
  char duration[24];

  stoptm = malloc(sizeof(char)*26);
  starttm = malloc(sizeof(char)*26);

  starttm = ctime_r(&tstart, starttm);
  stoptm = ctime_r(&tstop, stoptm);

  sprintf(duration, "%.2ld:%.2ld:%.2ld\n", (tstop - tstart)/3600, ((tstop - tstart)%3600)/60, (((tstop - tstart)%3600)%60));
  
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set(journal_list, &iter,
    JL_DURATION, duration,
    JL_TIME_START, starttm,
    JL_TIME_END, stoptm,
    JL_START_NOTES, istart,
    JL_END_NOTES, istop,
    -1);
  
  totaltime = totaltime + (tstop - tstart);
  g_free(starttm);
  g_free(stoptm);
}

/* add footer to the journal list */
void
journal_list_footer(int length)
{
  char duration[24];
  
  sprintf(duration, "%.2ld h. %.2ld m. %.2ld s.\n", (totaltime)/3600, ((totaltime)%3600)/60, (((totaltime)%3600)%60));
  gtk_list_store_append(journal_list, &iter);
  gtk_list_store_set (journal_list, &iter,
    JL_DURATION, _("Total time"),
    JL_TIME_START, duration,
    -1);
}
