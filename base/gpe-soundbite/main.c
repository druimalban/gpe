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

#include "gsm-codec.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { NULL, NULL }
};

#define _(x) gettext(x)

GtkWidget *window;
GtkWidget *progress_bar;

extern gboolean stop;
gboolean playing;
gboolean recording = -1;
GTimer *timer = NULL;
pid_t sound_process;

gchar *filename;

guint timeout_handler;

void
sigint (void)
{
  stop = TRUE;
}

void stop_sound (void)
{
  if (sound_process > 0)
    {
      kill (sound_process, SIGINT);
    }
  if (timer > 0)
    {
      g_timer_stop (timer);
    }

  if (timeout_handler != 0)
    {
      gtk_timeout_remove (timeout_handler);
    }
}

gint continue_sound (gpointer data)
{
  gdouble time;
  pid_t pid;
  time = g_timer_elapsed (timer, NULL);
  gtk_progress_configure (GTK_PROGRESS(progress_bar), time, 0.0, time);

  pid = waitpid (sound_process, NULL, WNOHANG);
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

  g_timer_start (timer);

  timeout_handler = gtk_timeout_add (150, continue_sound, NULL);

  /* it would be nicer if when we play back sound files we had a proper
     progress bar here - to do that we need to find out in advance how
     long (in seconds) the sound file is */

  gtk_progress_set_activity_mode (GTK_PROGRESS(progress_bar), TRUE);
  gtk_progress_set_format_string (GTK_PROGRESS(progress_bar), _("%v s"));
  gtk_progress_set_text_alignment (GTK_PROGRESS(progress_bar), 0.5, 0.5);
  gtk_progress_set_show_text (GTK_PROGRESS(progress_bar), TRUE);

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
                _exit(0);
              }
            else
              {
                sound_process = child;
              }
        }
        else
        {
	   if (infd < 0)
	     fprintf (stderr, "Error opening sound device for reading\n");
           if (outfd < 0)
             fprintf (stderr, "Error opening output file\n");
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
                _exit (0);
              }
            else
              {
                sound_process = child;
              }
        }
        else
        {
	   if (infd < 0)
	     fprintf (stderr, "Error opening input file '%s'\n", filename);
           if (outfd < 0)
             fprintf (stderr, "Error opening sound device for writing\n");
           exit(1);
        }
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
on_cancel_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  gtk_exit(0);
}

void
on_ok_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  gtk_exit(0);
}

int
main(int argc, char *argv[])
{
  GtkWidget *fakeparentwindow;
  GtkWidget *hbox, *vbox;
  GtkWidget *label;
  GtkWidget *buttonok, *buttoncancel;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  /* presumably this argument parsing should be done more nicely: */

  if (argc < 2)
    {
      fprintf (stderr, "must specify play or record\n");
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
      fprintf (stderr, "unknown command line option\n");
      exit(1);
    }

  /* and should obviously parse arguments for this! : */

  filename = strdup ("/tmp/out.gsm");

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  progress_bar = gtk_progress_bar_new ();
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

  gtk_widget_show (window);

  timer = g_timer_new ();

  start_sound ();

  gtk_main ();

  return 0;
}
