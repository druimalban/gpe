/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
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
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>

#include "init.h"
#include "picturebutton.h"
#include "pixmaps.h"
#include "render.h"
#include "gtkminifilesel.h"
#include "errorbox.h"

#include "gsm-codec.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { NULL, NULL }
};

#define _(x) gettext(x)

GtkWidget *window = NULL;
GtkWidget *progress_bar = NULL;
GtkWidget *file_selector = NULL;
gboolean gpe_initialised = FALSE;

extern gboolean stop;
gboolean playing = FALSE;
gboolean recording = FALSE;
GTimer *timer = NULL;
pid_t sound_process;

gchar *filename;

guint timeout_handler;

void
sigint (int signal)
{
  printf ("received sigint\n");
  stop = TRUE;
}

void exit_with_error (char *error)
{
  /* do we need this? : stop_sound (); */
  if (!gpe_initialised)
    {
      if (gpe_application_init (NULL, NULL) == FALSE)
        {
	  fprintf (stderr, "%s\n", error);
          exit (1);
        }
    }
  gpe_error_box (error);
  gtk_exit(1);
}

void stop_sound (void)
{
  if (sound_process > 0)
    {
      kill (sound_process, SIGINT);
      kill (sound_process, SIGKILL);
      waitpid (sound_process, 0, 1);
      sound_process = 0;
    }
/*  if (timer != NULL)
    {
      g_timer_stop (timer);
      timer = NULL;
    }

  if (timeout_handler != 0)
    {
      gtk_timeout_remove (timeout_handler);
    } */
}

gint continue_sound (gpointer data)
{
  gdouble time;
  pid_t pid;

  if (progress_bar)
  {
    time = g_timer_elapsed (timer, NULL);
    gtk_progress_configure (GTK_PROGRESS(progress_bar), time, 0.0, time);
  }

  pid = waitpid (sound_process, NULL, WNOHANG);
/*  printf ("waitpid in continue_sound gives %d\n", pid); */
  if (pid != 0)
  {
    stop_sound ();
    gtk_exit (0);
  }

  return TRUE;
}

void start_sound (void)
{
  stop_sound ();

  timeout_handler = gtk_timeout_add (150, continue_sound, NULL);

  /* it would be nicer if when we play back sound files we had a proper
     progress bar here - to do that we need to find out in advance how
     long (in seconds) the sound file is */

  if (recording)
    {
      int infd, outfd;

      infd = sound_device_open (O_RDONLY);
      outfd = creat (filename, S_IRWXU);

      if (infd >= 0 && outfd >= 0)
        {
	  pid_t child;
          child = fork();
          if (child == -1)
            {
	      perror ("fork");
	      exit (1);	
            }
          else
            if (child == 0)
              {
                signal (SIGINT, sigint);
                sound_encode (infd, outfd);
                exit(0);
              }
            else
              {
                sound_process = child;
              }
        }
        else
        {
	   if (infd < 0)
	     exit_with_error ("Error opening sound device for reading");
           if (outfd < 0)
             perror (filename);
           exit(1);
        }
    }
  else
    {
      int infd, outfd;
      infd = open (filename, O_RDONLY);
      outfd = sound_device_open (O_WRONLY);

      if (infd >= 0 && outfd >= 0)
        {
	  pid_t child;
          child = fork();
          if (child == -1)
            {
	      perror ("fork");
	      exit (1);	
            }
          else
            if (child == 0)
              {
                signal (SIGINT, sigint);
                sound_decode (infd, outfd);
                exit (0);
              }
            else
              {
                sound_process = child;
              }
        }
        else
        {
	   if (infd < 0)
             perror (filename);
           if (outfd < 0)
	     exit_with_error ("Error opening sound device for writing");
           exit(1);
        }
    }

  timer = g_timer_new ();
  g_timer_start (timer);

  if (window)
  {
    gtk_widget_show (window);
  }
}

void
on_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
  stop_sound ();
  gtk_exit (0);
}

void
on_key_release_signal                  (GtkWidget *widget,
                                            GdkEventKey *event,
                                            gpointer user_data)
{
  printf ("Key released: '%s'\n", gdk_keyval_name (event->keyval));
  if (!strcmp (gdk_keyval_name (event->keyval), "XF86AudioRecord"))
    {
      gdouble time;
      time = g_timer_elapsed (timer, NULL);
      if (time > 0.5) /* if less assume just mean to 'click' button */
        stop_sound ();
/*        gtk_exit (0); */
    }
}

void
on_cancel_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  remove (filename);
  gtk_exit(0);
}

void
on_ok_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  gtk_exit(0);
}

void
file_selector_destroy_signal (GtkFileSelection *selector, gpointer user_data)
{
  if (!filename)
  {
    gtk_exit(0);
  }
}

void
file_chosen_signal (GtkFileSelection *selector, gpointer user_data)
{
  filename = gtk_mini_file_selection_get_filename (GTK_MINI_FILE_SELECTION (file_selector));

  if (recording && !strchr (filename, '.'))
    {
      gchar *tmpfilename;
      tmpfilename = malloc (strlen(filename) + 4);
      sprintf (tmpfilename, "%s.gsm", filename);
      free (filename);
      filename = tmpfilename;
    }

  start_sound ();

  gtk_widget_destroy (file_selector);
}

void syntax_message (FILE *f)
{
      fprintf (f, "syntax: gpe-soundbite play|record [filename]\n");
}

int
main(int argc, char *argv[])
{
  GtkWidget *fakeparentwindow;
  GtkWidget *hbox, *vbox;
  GtkWidget *label;
  GtkWidget *buttonok, *buttoncancel;

  /* presumably this argument parsing should be done more nicely: */

  if (argc < 2)
    {
      syntax_message (stderr);
      exit (1);
    }

  if (!strcmp(argv[1], "record"))
    {
      recording = TRUE;
      playing = FALSE;
    }
  else if (!strcmp(argv[1], "play"))
    {
      playing = TRUE;
      recording = FALSE;
    }
  else
    {
      syntax_message (stderr);
      exit(1);
    }

  if (argc >= 3)
    {
      if (!strcmp (argv[2], "--autogenerate-filename"))
        {
          time_t time1;
          struct tm *time2;
          char *s;
          size_t length;
          time1 = time(NULL);
          time2 = localtime (&time1);
          length = strlen ("2002-06-23 03:54:00") + 1;
          s = g_malloc (length);
          length = strftime (s, length, "%Y-%m-%d %H:%M:%S", time2);
          if (length > 0)
            {
	      struct stat buf;
	      int i = 1;

              filename = g_strdup_printf ("%s/%s at %s", getenv("HOME"), _("Voice memo at"), s);
	      while (stat (filename, &buf) == 0)
	        {
		  i++;
		  g_free (filename);
                  filename = g_strdup_printf ("%s/%s at %s (%d)", getenv("HOME"), _("Voice memo at"), s, i);
                }
            }
        }
      else
        {
          filename = argv[2];
        }
    }
  else
    {
      filename = NULL;
    }

  if (filename)
    {
      start_sound ();
    }

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      stop_sound ();
      exit (1);
    }

  if (filename == NULL)
    {
      if (playing)
        file_selector = gtk_mini_file_selection_new ("Open audio note");
      else
        file_selector = gtk_mini_file_selection_new ("Save audio note as");

      gtk_signal_connect (GTK_OBJECT (file_selector),
                      "completed", GTK_SIGNAL_FUNC (file_chosen_signal), NULL);
      gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)->cancel_button),
                             "clicked", GTK_SIGNAL_FUNC (gtk_widget_destroy),
                             (gpointer) file_selector);
      gtk_signal_connect_object (GTK_OBJECT (GTK_MINI_FILE_SELECTION(file_selector)),
                             "destroy", GTK_SIGNAL_FUNC (file_selector_destroy_signal),
                             (gpointer) file_selector);
      gtk_widget_show (file_selector);
    }

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    {
      stop_sound ();
      gtk_exit (1);
    }

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  progress_bar = gtk_progress_bar_new ();
  gtk_progress_set_activity_mode (GTK_PROGRESS(progress_bar), TRUE);
  gtk_progress_set_format_string (GTK_PROGRESS(progress_bar), _("%v s"));
  gtk_progress_set_text_alignment (GTK_PROGRESS(progress_bar), 0.5, 0.5);
  gtk_progress_set_show_text (GTK_PROGRESS(progress_bar), TRUE);

  gtk_widget_show (progress_bar);
  if (playing)
    {
      label = gtk_label_new (_("Playing"));
    }
  else
    {
      label = gtk_label_new (_("Recording"));
    }
  gtk_widget_show (label);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox),
                      hbox, TRUE, FALSE, 0);

  gtk_widget_realize (window);

  buttonok = gpe_picture_button (window->style, _("OK"), "ok");
  gtk_signal_connect (GTK_OBJECT (buttonok), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_button_clicked),
                      NULL);

  buttoncancel = gpe_picture_button (window->style, _("Cancel"), "cancel");
  gtk_signal_connect (GTK_OBJECT (buttoncancel), "clicked",
                      GTK_SIGNAL_FUNC (on_cancel_button_clicked),
                      NULL);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
                     buttoncancel, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area),
                      buttonok, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (on_window_destroy),
                      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "key-release-event",
                      GTK_SIGNAL_FUNC (on_key_release_signal),
                      NULL);

  if (filename)
    {
      gtk_widget_show (window);
      gtk_widget_add_events (window, GDK_KEY_RELEASE_MASK);
      gtk_widget_grab_focus (GTK_DIALOG (window)->action_area);
    }

  gtk_main ();

  return 0;
}
