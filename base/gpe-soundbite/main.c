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

#include <sys/vfs.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include <gpe/init.h>
#include <gpe/picturebutton.h>
#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>

#include "gsm-codec.h"

static struct gpe_icon my_icons[] = {
  { "ok", "ok" },
  { "cancel", "cancel" },
  { NULL, NULL }
};

#define _(x) gettext(x)

GtkWidget *window = NULL; /* dialog window to show status */
GtkWidget *progress_bar = NULL; /* progress/activity indicator */
GtkWidget *label = NULL; /* label for progress_bar */
GtkWidget *file_selector = NULL;

gboolean gpe_initialised = FALSE;

extern gboolean stop; /* set to TRUE when the sound process is trying to end */
gboolean playing = FALSE; /* are we playing back a recording? */
gboolean streaming = FALSE; /* are we sending sound to stdout? */
gboolean recording = FALSE; /* (or) are we making a new recording */
GTimer *timer = NULL; /* timer used to provide callbacks */
pid_t sound_process; /* process ID of child that plays or records */

gdouble playlength; /* length in seconds of file we're playing */

gchar *filename;

guint timeout_handler;

void
sigint (int signal)
{
  fprintf (stderr, "received sigint\n");
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
      fprintf (stderr, "signalling child sound process\n");
      kill (sound_process, SIGINT);
      kill (sound_process, SIGKILL);
      if (waitpid (sound_process, 0, 1) == sound_process)
        sound_process = 0;
      else
        fprintf (stderr, "failed to kill sound process\n");
    }
  else
    fprintf (stderr, "no sound process found\n");
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
      if (recording)
        {
	  struct statfs buf;
	  long available_time;
          gtk_progress_configure (GTK_PROGRESS(progress_bar), time, 0.0, time);
	  if (statfs (filename, &buf) != 0)
	    perror ("statfs");
          /* fprintf (stderr, "%ld KB remaining\n", buf.f_bavail * (buf.f_bsize >> 10)); */
	  available_time = (long) (((double) (buf.f_bavail * buf.f_bsize)) / 1650.*2.);
	  if (available_time < 7200)
	    {
	      gtk_label_set_text (GTK_LABEL(label), g_strdup_printf ("%s (%02ld:%02ld %s)", _("Recording"), available_time / 60, available_time % 60, _("available")));
            }
        }
      else
        {
          if (time >= playlength)
	    {
	      stop_sound ();
              gtk_exit (0);
            }
          gtk_progress_configure (GTK_PROGRESS(progress_bar), time, 0.0, playlength);
        }
    }

  if (sound_process)
    {
      pid = waitpid (sound_process, NULL, WNOHANG);
      if (pid != 0)
        {
          if (pid < 0)
            perror ("waitpid");
          stop_sound ();
          if (recording)
            gtk_exit (0);
          else
            sound_process = 0;
        }
    }

  return TRUE;
}

void start_sound (void)
{
  stop_sound ();

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
                close (infd);
                close (outfd);
		fprintf (stderr, "child sound process is exiting\n");
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
  else /* playing */
    {
      int infd, outfd;
      struct stat buf;

      if (stat (filename, &buf) != 0)
	perror ("stat");
      playlength = buf.st_size / (1650*2);

      infd = open (filename, O_RDONLY);
      if (streaming)
	{
	  outfd = 1;
	}
      else
	{
          outfd = sound_device_open (O_WRONLY);
	}

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
                close (infd);
                close (outfd);
		fprintf (stderr, "child sound process is exiting\n");
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

  timeout_handler = gtk_timeout_add (150, continue_sound, NULL);

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
  fprintf (stderr, "Key released: '%s'\n", gdk_keyval_name (event->keyval));
  if (!strcmp (gdk_keyval_name (event->keyval), "XF86AudioRecord"))
    {
      gdouble time;
      time = g_timer_elapsed (timer, NULL);
      if (time > 0.5) /* if less assume just mean to 'click' button */
	{
	  fprintf (stderr, "should now stop\n");
          stop_sound ();
          gtk_exit (0);
	}
      else
	{
	  fprintf (stderr, "assuming key bounced, so not stopping\n");
	}
    }
}

void
on_cancel_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  if (recording)
    remove (filename);
  fprintf (stderr, "cancelled\n");
  gtk_exit(0);
}

void
on_ok_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
  stop_sound();
  fprintf (stderr, "exiting\n");
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
  filename = g_strdup (gtk_file_selection_get_filename (GTK_FILE_SELECTION (file_selector)));

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
      fprintf (f, "usage: gpe-soundbite play|record [filename]\n       gpe-soundbite play|stream|record --autogenerate-filename pathname\n       gpe-soundbite --help\n");
}

int
main(int argc, char *argv[])
{
  GtkWidget *fakeparentwindow;
  GladeXML *xml;
  char *gladefile;

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
      streaming = FALSE;
      recording = FALSE;
    }
  else if (!strcmp(argv[1], "stream"))
    {
      playing = TRUE;
      streaming = TRUE;
      recording = FALSE;
    }
  else if (!(strcmp(argv[1], "help") && strcmp(argv[1], "--help") && strcmp(argv[1], "-h")))
    {
      syntax_message (stdout);
      exit (0);
    }
  else
    {
      syntax_message (stderr);
      exit (1);
    }

  if (argc >= 3)
    {
      if (!strcmp (argv[2], "--autogenerate-filename"))
        {
          time_t time1;
          struct tm *time2;
	  char *savedir = NULL;
          char *s;
          size_t length;

	  if (argc >= 4)
	    {
	      savedir = argv[3];
              if (savedir == NULL)
	        {
                  syntax_message (stderr);
	          exit (1);
                }
	    }
	  else
	    {
	      savedir = getenv("HOME");
              if (savedir == NULL)
	        {
                  fprintf (stderr, "$HOME does not contain the path to the user's home direcory!\n");
		  exit (1);
                }
	    }

          time1 = time(NULL);
          time2 = localtime (&time1);
          length = strlen ("2002-06-23 03:54:00") + 1;
          s = g_malloc (length);
          length = strftime (s, length, "%Y-%m-%d %H:%M:%S", time2);
          if (length > 0)
            {
	      struct stat buf;
	      int i = 1;

              filename = g_strdup_printf ("%s/%s at %s", savedir, _("Voice memo at"), s);
	      while (stat (filename, &buf) == 0)
	        {
		  i++;
		  g_free (filename);
                  filename = g_strdup_printf ("%s/%s at %s (%d)", savedir, _("Voice memo at"), s, i);
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

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  if (gpe_application_init (&argc, &argv) == FALSE)
    {
      exit (1);
    }

  if (filename)
    {
      start_sound ();
    }

  if (filename == NULL)
    {
      if (playing)
        file_selector = gtk_file_selection_new ("Open audio note");
      else
        file_selector = gtk_file_selection_new ("Save audio note as");

      gtk_signal_connect (GTK_OBJECT (file_selector),
                      "completed", GTK_SIGNAL_FUNC (file_chosen_signal), NULL);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                        "clicked",
                        GTK_SIGNAL_FUNC (file_chosen_signal),
                        NULL);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->ok_button),
                          "clicked",
                          GTK_SIGNAL_FUNC (gtk_widget_destroy),
                          (gpointer) file_selector);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
                          "clicked",
                          GTK_SIGNAL_FUNC (file_selector_destroy_signal),
                          (gpointer) file_selector);

      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION(file_selector)),
                             "destroy", GTK_SIGNAL_FUNC (file_selector_destroy_signal),
                             (gpointer) file_selector);

      gtk_widget_show (file_selector);
    }

  if (gpe_load_icons (my_icons) == FALSE)
    {
      stop_sound ();
      gtk_exit (1);
    }

    gladefile = g_build_filename (PREFIX, "/share/gpe-soundbite",
                  "gpe-soundbite.glade", NULL);

    xml = glade_xml_new(gladefile, NULL, NULL);

    window = glade_xml_get_widget(xml, "window");
    progress_bar = glade_xml_get_widget(xml, "progressbar");
    label = glade_xml_get_widget(xml, "label");

    glade_xml_signal_autoconnect(xml);

/*
  fakeparentwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (fakeparentwindow);

  window = gtk_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW(window), GTK_WINDOW(fakeparentwindow));
  gtk_widget_realize (window);
*/
  
  gtk_progress_set_format_string (GTK_PROGRESS(progress_bar), _("%v s"));
  gtk_progress_set_text_alignment (GTK_PROGRESS(progress_bar), 0.5, 0.5);
  gtk_progress_set_show_text (GTK_PROGRESS(progress_bar), FALSE);

  if (playing)
    {
      gtk_progress_set_activity_mode (GTK_PROGRESS(progress_bar), FALSE);
      gtk_window_set_title (GTK_WINDOW(window), "Play Memo");
      gtk_label_set_text (GTK_LABEL(label), _("Playing"));
    }
  else
    {
      gtk_progress_set_activity_mode (GTK_PROGRESS(progress_bar), TRUE);
      gtk_window_set_title (GTK_WINDOW(window), "Record Memo");
      gtk_label_set_text (GTK_LABEL(label), _("Recording"));
    }

  if (filename)
    {
      gtk_widget_show (window);
      gtk_widget_add_events (window, GDK_KEY_RELEASE_MASK);
      gtk_widget_grab_focus (GTK_DIALOG (window)->action_area);
    }

  gtk_main ();

  return 0;
}
