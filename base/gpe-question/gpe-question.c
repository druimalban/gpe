/*
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/* Example:
 *
 * gpe-question --question "Question?" --buttons cancel:Cancel "ok:Go ahead"
*/

#include <fcntl.h>
#include <libintl.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/question.h>

#define _(x) gettext(x)

#define MAXBUTTONS 10

int
main(int argc, char *argv[])
{
  gint i, answer = -1;
  gchar *question = NULL;
  gchar *icon[MAXBUTTONS];
  gchar *text[MAXBUTTONS];
  gchar *the_icon = "question";

  for (i = 0; i < MAXBUTTONS; i++)
    {
      icon[i] = text[i] = NULL;
    }

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--icon"))
	{
	  the_icon = argv[++i];
	  continue;
	}
      if (!strcmp (argv[i], "--question"))
	{
	  question = argv[++i];
	  continue;
	}
      if (!strcmp (argv[i], "--buttons"))
	{
	  int j = 0;
	  for (i++; i < argc; i++)
	    {
	      gchar *colon;
	      text[j] = g_strdup (argv[i]);
	      if ((colon = strchr (text[j], ':')) && (colon[-1] != '\\'))
	  	{
		  *colon = 0;
		  icon[j] = text[j];
		  text[j] = &colon[1];
		}
	      else
		icon[j] = NULL;
	      j++;
	    }
	  continue;
	}
    }

  if ((question == NULL) || (text[0] == NULL))
    {
      fprintf (stderr, "Syntax: gpe-question [--icon name] --question \"Question text\" --buttons [icon1:]text1 [[icon2:]text2] [...]\n");
      return -1;
    }

  answer = gpe_question_ask (question, "", the_icon,
                             text[0], icon[0],
                             text[1], icon[1],
                             text[2], icon[2],
                             text[3], icon[3],
                             text[4], icon[4],
                             text[5], icon[5],
                             text[6], icon[6],
                             text[7], icon[7],
                             text[8], icon[8],
                             text[9], icon[9],
			     NULL, NULL);

  return answer;
}
