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
#include <stdio.h>
#include <gdk/gdk.h>
#include <gpe/errorbox.h>
#include <libintl.h>
#include <unistd.h>
#include <time.h>

#include "sql.h"
#include "html.h"

#define _(x) gettext(x)
#define FILE_URL_PREFIX "file://"
#define HTML_APP1 "/usr/bin/dillo"
#define HTML_APP2 "/usr/bin/minimo"

static char **myjournal = NULL;
static int jlen = 0;

/* adds the document header */
int 
journal_add_header(char* title)
{
  /* if you change len here, change return value too */
  myjournal = malloc(5 * sizeof(char*));
  if (!myjournal) 
    return -1;
  myjournal[0] = 
    g_strdup_printf("<html>\n<head>\n<title>%s</title>\n</head>",
      title);
  myjournal[1] = g_strdup_printf("<body>\n<h1>%s %s</h1>\n<p>",
    _("Journal for"), title);
  myjournal[2] = g_strdup("<table WIDTH=100% BORDER=0 CELLPADDING=0 CELLSPACING=0>\n");
  myjournal[3] = g_strdup_printf("<tr><th align=\"left\">%s</th><th " \
    "align=\"left\">%s</th><th align=\"left\">%s</th><th align=\"left\">%s</th></tr>",
  	_("Start"), _("Comment"), _("End"), _("Duration"));
  myjournal[4] = NULL;
  jlen = 5;
  return jlen; /* curent length */

}


/* this one adds a data line to journal list */
int 
journal_add_line(time_t tstart, time_t tstop, 
                 const char *istart, const char *istop)
{
  myjournal = realloc(myjournal,sizeof(char*)*(++jlen));
  myjournal[jlen - 2] = 
    g_strdup_printf("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
	  ctime(&tstart), istart, ctime(&tstop), istop);
  myjournal[jlen - 1] = NULL;
  return jlen;
}


/* add a footer and closing tags to the document */
int 
journal_add_footer(int length)
{
  myjournal = realloc(myjournal,sizeof(char*)*(++jlen));
  myjournal[jlen - 2] = 
    g_strdup_printf("</table>\n</p>\n</body>\n</html>");
  myjournal[jlen - 1] = NULL;
  return jlen;
}


/* dump string vector to file */
int 
journal_to_file(const char *filename)
{
  int i = 0;
  FILE *fd;
  
  fd = fopen(filename,"w");
  if (fd == NULL)
    {
       gpe_error_box(_("Could not create output file!"));
       return -1;
    }
  while (myjournal[i])
    fprintf(fd,"%s\n",myjournal[i++]);
  
  fclose(fd);
  g_strfreev(myjournal);
  jlen = 0;
  myjournal = NULL;
  return 0;
}


/* display output file in html browser */
int 
journal_show(const char *filename)
{
  char *app = NULL;
  char *jurl;
  pid_t p_html;
	
  /* check if the file is readable */
  if (access(filename,R_OK))
    return -1;
	
  /* check if we are able to execute one of the display
  applications */
  if (!access(HTML_APP1,X_OK)) app = HTML_APP1;
    else
       if (!access(HTML_APP2,X_OK)) app = HTML_APP2;
	
  /* return if no app is available */
  if (app == NULL) 
    return -2;
	
  /* construct the complete file url */
  jurl = g_strdup_printf("%s%s", FILE_URL_PREFIX, filename);

  /* fork and exec display application */
  p_html = fork();
  switch (p_html)
    {
      case -1: 
        return -3;
      break;
      case  0: 
        execlp(app,app,jurl,NULL);
      break;
      default: 
        g_free(jurl);
        return 0;
      break;
    } 
  /* we should never get there, 
  help the compiler - he doesn't know */
  return -1;
}
