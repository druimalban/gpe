/* gtk_timer.c 
 * This file contains some simple functions for starting and stopping 
 * one or more timers in a GTK application, and tying a function to
 * each timer.
 */ 

#include <stdio.h>
#include <gtk/gtk.h>
#include "gtk_timer.h"  /* contains the "timer_data" data type */


void initialize_timer (type_timer *new_timer, GtkWidget *label, 
		       char *description)
{
  new_timer->interval_count = 0;
  new_timer->label = label;
  new_timer->interval_description = description;
  new_timer->is_running = FALSE;
}


gint timer_callback (gpointer timer_ptr)
{
  char buffer[50];
  type_timer *app_timer;

  app_timer = (type_timer *) timer_ptr;

  if (app_timer->is_running)
    {
      app_timer->interval_count++;
      sprintf (buffer, "%d %s", app_timer->interval_count, 
	       app_timer->interval_description);
      gtk_label_set (GTK_LABEL (app_timer->label), buffer);
    }
  return (1);
}


void reset_timer (type_timer *app_timer, int starting_value)
{
  char buffer[50];

  app_timer->interval_count = starting_value;
  sprintf (buffer, "%d %s", app_timer->interval_count, 
	   app_timer->interval_description);
  gtk_label_set (GTK_LABEL (app_timer->label), buffer);
}


void start_timer (type_timer *app_timer, int ms_timeout)
{
  if (!app_timer->is_running)
    {
      app_timer->timer = gtk_timeout_add (ms_timeout, timer_callback, 
					  (gpointer) app_timer);
      app_timer->is_running = TRUE;
    }
}


void stop_timer (type_timer *app_timer)
{
  if (app_timer->is_running)
    {
      gtk_timeout_remove (app_timer->timer);
      app_timer->is_running = FALSE;
    }
}


int timer_is_running (type_timer *app_timer)
{
  return (app_timer->is_running);
}


int timer_current_value (type_timer *app_timer)
{
  return (app_timer->interval_count);
}
