/*
 * gpe-helpviewer
 *
 * Copyright (c) 2004 Phil Blundell
 * Copyright (c) 2004 Philippe De Swert, for Aleph One Ltd.
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

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtkhtml/gtkhtml.h>

struct button_data {	GtkWidget *web;
						GList *list;
					}buttond;
GList *history;  //keep a list of urls
GList *lasthistory=NULL;  //keep last position in the list
gchar *lastdata; //keep last url

void
read_file (const gchar *filename, GtkWidget *html)
{
  FILE *fp;
  GtkHTMLStream *stream;

  stream = gtk_html_begin (GTK_HTML (html));

  fp = fopen (filename, "r");
  if (fp)
    {
      char buf[1024];
      while (!feof (fp))
	{
	  int n = fread (buf, 1, sizeof (buf), fp);
	  gtk_html_write (GTK_HTML (html), stream, buf, n);
	}
      fclose (fp);
    }
  else
    {
      fprintf (stderr, "Could not open %s\n", filename);
	  read_file ("/usr/share/help/doc-not-installed.html", html);
    }

  gtk_html_end (GTK_HTML (html), stream, GTK_HTML_STREAM_OK);
}

static void
url_requested (GtkHTML *html, const char *url, GtkHTMLStream *handle, gpointer data)
{
  FILE *fp;
  const gchar *p;

  p = strchr (url, ':');
  if (p)
    p = p + 1;
  else
    p = url;

  fp = fopen (p, "r");
  if (fp)
    {
      char buf[1024];
      while (!feof (fp))
	{
	  int n = fread (buf, 1, sizeof (buf), fp);
	  gtk_html_write (html, handle, buf, n);
	}
      fclose (fp);
    }

  gtk_html_end (html, handle, GTK_HTML_STREAM_OK);
}

static void
link_clicked (GtkHTML *html, const gchar *url, gpointer data)
{
  const gchar *p;
  GList *temp;
  int i=0;
  
	  	p = strchr (url, ':');
				if (p)
    			 p = p + 1;
  				else
    			 p = url;
  lastdata = g_strdup (p);		
  //add url to list and check if we have been going back earlier
  //if the back button has been pressed clean up list to save memory 
  //and to go back to the right page
  if (lasthistory != g_list_last(history) && lasthistory != NULL)
		  {
				 temp = lasthistory;
				 while (g_list_next(temp) != NULL)
				 {
					 i++;
				 	 temp = g_list_next(temp);
				 }
				 while (i > 0)
				 {
				 	history = g_list_remove(history, temp->data);
					temp = g_list_last(history);
					i--;
				}
		  }
  history = g_list_append(history, (gpointer *) lastdata );
  lasthistory = g_list_last(history);
  read_file (p, GTK_WIDGET (html));
}
	
/*static void
find_func ()
{
}*/

static void
forward_func (GtkWidget *forward, GtkWidget *html)
{
	const gchar *goforward;
	
	//if there is a next page in the list show it, otherwise do nothing
	if (g_list_next(lasthistory) != NULL)
			{
			goforward = g_list_next(lasthistory)->data;
			lasthistory = g_list_next(lasthistory);
			read_file ( goforward, GTK_WIDGET (buttond.web));
			}

}

static void
back_func (GtkWidget *back, GtkWidget *web)
{
	const gchar *goback;
	
	//if there no previous list stop going back
	if (g_list_previous(lasthistory)!= NULL)
	{	
	    goback = g_list_previous(lasthistory)->data;
		lasthistory = g_list_previous(lasthistory);
		read_file (goback, GTK_WIDGET (buttond.web));
	}
	else
	{
		g_message ("no more pages back!");
	}
}

static void
home_func (GtkWidget *home, GtkWidget *html)
{
	//append home page to list, so we can get back to home using the back button
	history = g_list_append(history, "/usr/share/help/help-index.html");
	//set lasthistory to last element in list, otherwise we go back one
	//page to much after pressing home.
  	lasthistory = g_list_last(history);
	read_file ("/usr/share/help/help-index.html", GTK_WIDGET (html));
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *html;
  GtkWidget *app;
  GtkWidget *contentbox;
  GtkWidget *toolbar;
  GtkWidget *iconw;
  GtkWidget *back_button, *forward_button, *home_button, *search_button;
  gchar *base;
  const gchar *p;
  
  gtk_init (&argc, &argv);

  if (argc != 2)
    {
	  argv[1] = "/usr/share/help/help-index.html";
	  /* if no url is given load the standard url, the rest of the args are not used
	   * and thus ignored */
    }
  //check if the help url starts with file:// and remove it if needed
    p = strchr (argv[1], ':');
	  if (p)
		      p = p + 3;
	    else
			    p = argv[1];
  base = p;		
  
  fprintf( stderr, "url = %s\n", base);
  //create list and add first element from the command line
  history = NULL;
  history = g_list_append(history, base);


  //create application window
  app = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (app), "delete-event", gtk_main_quit, NULL);
  gtk_window_set_default_size (GTK_WINDOW (app), 800, 600);
  gtk_window_set_title (GTK_WINDOW (app), "Help viewer");	
  gtk_widget_realize (app);
  
  //create boxes
  contentbox = gtk_vbox_new(FALSE,0);
  
  //create toolbar and add to topbox
  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_BOTH);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  //create html object (must be created before function calls to html to avoid segfault)
  html = gtk_html_new ();

  //buttond = (struct button_data *) malloc (sizeof(struct button_data));
  buttond.web = html;
  buttond.list = history;
  
  //add home, search, back and forward button
  iconw = gtk_image_new_from_file ("/usr/share/help/back.png"); /* icon widget */
  back_button =  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), /* our toolbar */
				                          "Back",               /* button label */
				                          "Go back a page",     /* this button's tooltip */
				                          "Back",             /* tooltip private info */
				                          iconw,                 /* icon widget */
				                          GTK_SIGNAL_FUNC (back_func), /* a signal */
				                          html);
  iconw = gtk_image_new_from_file ("/usr/share/help/forward.png"); 
  forward_button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
		  									"Forward", 
											"Next page", 
											"Forward", 
											iconw, 
											GTK_SIGNAL_FUNC (forward_func), 
											html);
 /* iconw = gtk_image_new_from_file ("help/search.png"); 
  search_button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
		  									"Find",
											"Find a word in the text",
											"Find",
											iconw,
											GTK_SIGNAL_FUNC (find_func),
											NULL);*/
  iconw = gtk_image_new_from_file ("/usr/share/help/home.png");
  home_button = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
		  									"Home",
											"Go to the help index",
											"Home",
											iconw,
											GTK_SIGNAL_FUNC (home_func),
											html);
  
  gtk_toolbar_append_space (GTK_TOOLBAR (toolbar)); /* space after item */
  gtk_widget_show (toolbar); 
  
  //create scrolling window to display html in
  window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  
  //create html reader
  gtk_widget_show (html);


  g_signal_connect (G_OBJECT (html), "url_requested", 
		    G_CALLBACK (url_requested), html);

  g_signal_connect (G_OBJECT (html), "link_clicked", 
		    G_CALLBACK (link_clicked), history);
  
  //make everything viewable
  gtk_container_add (GTK_CONTAINER (window), html);
  
  gtk_box_pack_start (GTK_BOX(contentbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(contentbox), window, TRUE, TRUE, 0);
	  
  gtk_container_add(GTK_CONTAINER (app), contentbox);

  read_file (base, html);

  gtk_widget_show (window);
  gtk_widget_show (contentbox);
  gtk_widget_show (app);
  gtk_main ();

  exit (0);
}
