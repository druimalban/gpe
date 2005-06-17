/*
 * gpe-helpviewer v0.5
 *
 * Displays HTML help files 
 *
 * Copyright (c) 2004 Phil Blundell
 * Copyright (c) 2004 Philippe De Swert, for Aleph One Ltd.
 * Copyright (c) 2005 Philippe De Swert
 *
 * Contact : philippedeswert@scarlet.be
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <webi.h>

#include <gdk/gdk.h>

#include <glib.h>
#include <gpe/init.h>
#include <gpe/errorbox.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define SOCKETNAME "/tmp/comm"
#define EC_FAIL 1
//#define DEBUG /* uncomment this if you want debug output*/

struct button_data
{
  GtkWidget *web;
  GList *list;
} buttond;
GList *history;			//keep a list of urls
GList *lasthistory = NULL;	//keep last position in the list
gchar *lastdata;		//keep last url

/* read a file into gtk-webcore */
void
read_file (const gchar * url, GtkWidget * html)
{
  const gchar *filename;

  filename = strchr (url, ':');
  if (filename)
    filename = filename + 3;

  if (!access (filename, F_OK))
    {
#ifdef DEBUG
      fprintf (stderr, "loading html %s\n", filename);
#endif
      webi_stop_load (WEBI (html));
      webi_load_url (WEBI (html), url);
    }
  else
    {
#ifdef DEBUG
      fprintf (stderr, "Could not open %s\n", filename);
#endif
      if (!access ("/usr/share/doc/gpe/doc-not-installed.html", F_OK))
	read_file ("file:///usr/share/doc/gpe/doc-not-installed.html", html);
      else
	{
	  gpe_error_box ("Help not installed or file not found");
	}
    }
}

/* parse url to something easy to interprete */
const gchar *
parse_url (const gchar * url)
{
  const gchar *p;


  p = strchr (url, ':');
  if (p)
    {
      return p;
    }
  else
    {
      p = g_strconcat ("file://", url, NULL);
    }
  return (p);
}

/*static void
find_func ()
{
}*/

/* makes the engine go forward one page */
static void
forward_func (GtkWidget * forward, GtkWidget * html)
{

  if (webi_can_go_forward (WEBI (html)))
    webi_go_forward (WEBI (html));
  else
    gpe_error_box ("no more pages forward!");

}

/* makes the engine go back one page */
static void
back_func (GtkWidget * back, GtkWidget * html)
{
  if (webi_can_go_back (WEBI (html)))
    webi_go_back (WEBI (html));
  else
    gpe_error_box ("No more pages back!");


}

/* makes the engine load the help index page, if none exists it tries to generate it */
static void
home_func (GtkWidget * home, GtkWidget * html)
{
  if (!access ("/usr/share/doc/gpe/help-index.html", F_OK))
    system ("gpe-helpindex");
  read_file ("file:///usr/share/doc/gpe/help-index.html", GTK_WIDGET (html));
}

/* open socket, check for instance and send it a quit message */
int
checkinstance (const gchar * url)
{
  struct sockaddr_un sv;
  int fd_skt, n, retry = 0;
  char buf[100];

  strcpy (sv.sun_path, SOCKETNAME);
  sv.sun_family = AF_UNIX;
  fd_skt = socket (AF_UNIX, SOCK_STREAM, 0);
#ifdef DEBUG
  while (connect (fd_skt, (struct sockaddr *) &sv, sizeof (sv)) == -1)
    {
      if ((errno != ENOENT) && (retry < 1))
	{
	  sleep (1);
	  printf ("retry!\n");
	  retry++;
	  continue;
	}
      else
	{
	  return (0);
	}
    }
#endif
#ifndef DEBUG
  connect (fd_skt, (struct sockaddr *) &sv, sizeof (sv));
#endif
  n = strlen (url);
  //write (fd_skt, url, n+1);
  write (fd_skt, "quit", 5);
  read (fd_skt, buf, sizeof (buf));
  close (fd_skt);
  return (0);
}

/* open listening socket for single instance. The application quits when it gets a message*/
static void *
reportinstance (void *html2)
{
  int fd_skt, fd_client, len;
  char buf[100];
  struct sockaddr_un skt;

  while (1)
    {
      (void) unlink (SOCKETNAME);
      strcpy (skt.sun_path, SOCKETNAME);
      skt.sun_family = AF_UNIX;
      fd_skt = socket (AF_UNIX, SOCK_STREAM, 0);
      if (bind (fd_skt, (struct sockaddr *) &skt, sizeof (skt)) != 0)
	printf ("bind failed \n");
      if (listen (fd_skt, SOMAXCONN) != 0)
	printf ("listening failed\n ");
      fd_client = accept (fd_skt, NULL, 0);
      read (fd_client, buf, sizeof (buf));
      write (fd_client, "Goodbye!", 9);
      close (fd_client);
      close (fd_skt);
      exit (0);
    }
}

int
main (int argc, char *argv[])
{
  GtkWidget *html;
  GtkWidget *app;
  GtkWidget *contentbox;
  GtkWidget *toolbar;
  GtkWidget *iconw;
  GtkWidget *back_button, *forward_button, *home_button, *search_button,
    *exit_button;
  const gchar *base;
  gchar *p;
  gint width = 240, height = 320;
  pthread_t tid;

  WebiSettings s = { 0, };
  WebiSettings *ks = &s;

  gpe_application_init (&argc, &argv);
  checkinstance (argv[1]);
#ifdef DEBUG
  printf ("no instance running!\n");
#endif

  if (argc == 1)
    {

      printf
	("GPE-helpviewer, basic help viewer application. (c)2005, Philippe De Swert\n");
      printf ("Usage: gpe-helpviewer /path/to/file\n");
      exit (0);
    }

  base = parse_url (argv[1]);
#ifdef DEBUG
  fprintf (stderr, "url = %s\n", base);
#endif
  //create list and add first element from the command line
  history = NULL;
  history = g_list_append (history, (gchar *) base);


  //create application window
  app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (app), "delete-event", gtk_main_quit, NULL);
  width = gdk_screen_width ();
  height = gdk_screen_height ();
  gtk_window_set_default_size (GTK_WINDOW (app), width, height);
  gtk_window_set_title (GTK_WINDOW (app), "Help viewer");
  gtk_window_set_resizable (GTK_WINDOW (app), TRUE);
  gtk_widget_realize (app);

  //create boxes
  contentbox = gtk_vbox_new (FALSE, 0);

  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  //create html object (must be created before function calls to html to avoid segfault)
  html = webi_new ();
  webi_set_emit_internal_status (WEBI (html), TRUE);

  ks->default_font_size = 11;
  ks->default_fixed_font_size = 11;
  webi_set_settings (WEBI (html), ks);

  pthread_create (&tid, NULL, reportinstance, html);
#ifdef DEBUG
  printf ("report running! html=%d\n", html);
  printf ("continuing!\n");
#endif

  //buttond = (struct button_data *) malloc (sizeof(struct button_data));
  buttond.web = html;
  buttond.list = history;

  /*add home, search, back, forward and exit button */
  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_BACK,
			    ("Go back a page"), ("Back"),
			    GTK_SIGNAL_FUNC (back_func), html, -1);

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_GO_FORWARD,
			    ("Go to the next page"), ("Next"),
			    GTK_SIGNAL_FUNC (forward_func), html, -1);

  /* TODO: Actually implement search function 
     iconw = gtk_image_new_from_file ("help/search.png"); 
     search_button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), "Find", "Find a word in the text","Find",
     iconw, GTK_SIGNAL_FUNC (find_func), NULL); */

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_HOME,
			    ("Go to help home"), ("Home"),
			    GTK_SIGNAL_FUNC (home_func), html, -1);

  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));	/* space after item */

  gtk_toolbar_insert_stock (GTK_TOOLBAR (toolbar), GTK_STOCK_QUIT,
			    ("Exit help"), ("Exit"),
			    GTK_SIGNAL_FUNC (gtk_main_quit), NULL, -1);

  gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar),
			     GTK_ICON_SIZE_SMALL_TOOLBAR);
  /* only show icons if the screen is 240x320 | 320x240 or smaller */
  if ((width <= 240) || (height <= 240))
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_widget_show (toolbar);

  //create html reader
  gtk_widget_show_all (html);

  //make everything viewable
  gtk_box_pack_start (GTK_BOX (contentbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (contentbox), html, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (app), contentbox);

  read_file (base, html);

  gtk_widget_show (GTK_WIDGET (contentbox));
  gtk_widget_show (GTK_WIDGET (app));
  gtk_main ();

  exit (0);
}
