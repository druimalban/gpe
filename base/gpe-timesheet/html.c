/*
 * This file is part of gpe-timeheet
 * (c) 2004 Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 */

#include <stdlib.h>
#include <gdk/gdk.h>
#include <libintl.h>
#include "sql.h"
#include "html.h"

#define _(x) gettext(x)

/* awaits pointer to empty list of char[] */
int 
journal_add_header(char* title, char ***buffer)
{
  /* if you change len here, change return value too */
  *buffer = malloc(3 * sizeof(char*));
  if (!buffer) 
    return -ENOMEM;
  *buffer[0] = g_strdup_printf("<html>\n<head>\n<title>%s</title>\n</head>",
    title);
  *buffer[1] = g_strdup_printf("<body>\n<h1>%s %s</h1>\n<p>",
    _("Journal for "), title);
  *buffer[2] = NULL;
  return 3; /* curent length */
}


/* this one adds a data line to given list */
int 
journal_add_line(struct task atask, char ***buffer, int length)
{
  *buffer = realloc(*buffer,sizeof(char*)*++length);
  buffer[length - 2] = 
    g_strdup_printf("%s %s<br>\n",atask.description, atask.time_cf);
  buffer[length - 1] = NULL;
  return length;
}


/* add a footer and closing tags to the document */
int 
journal_add_footer(char ***buffer, int length)
{
  *buffer = realloc(*buffer,sizeof(char*)*++length);
  buffer[length - 2] = 
    g_strdup_printf("</p>\n</body>\n</html>");
  buffer[length - 1] = NULL;
  return length;
}


/* dump string vector to file */
int 
journal_to_file(char **buffer, char *filename)
{

}


/* display output file in html browser */
int 
journal_show(char *filename)
{
	
}
