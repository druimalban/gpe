/*
 * Copyright (C) 2002 Moray Allan <moray@sermisy.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
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

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "question", "question" },
  { NULL, NULL }
};

#define _(x) gettext(x)

int
main(int argc, char *argv[])
{
  gint answer = -1;

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    {
      gtk_exit (1);
    }

  answer = 0; /* gpe_question_ask ("my question", "my title", "a"); */

/*  gtk_main (); */

  return answer;
}
