#ifndef _CALLBACKS_H
#define _CALLBACKS_H

#include <gtk/gtk.h>

gchar*
change_scale_label (GtkScale *scale, gdouble val);

gchar*
change_dim_scale_label (GtkScale *scale, gdouble val);

void
start_button (GtkButton       *button,
	      gpointer         user_data);

void
stop_button (GtkButton       *button,
	     gpointer         user_data);

void
mainquit (GtkButton       *button,
	  gpointer         user_data);

void
AS_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data);

void
AD_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data);

void
cpu_checked (GtkToggleButton *togglebutton,
	     gpointer         user_data);

void
irq_choose_but (GtkButton       *button,
		gpointer         user_data);

void
on_dim_scale_changed (GtkRange  *range, gpointer user_data);

void
irq_done_clicked (GtkButton       *button,
		  gpointer         user_data);

void
on_sleep_idle_spin_activate (GtkRange     *range,
			     gpointer         user_data);

void
on_sleep_idle_spin_changed (GtkRange     *range,
			    gpointer         user_data);

void
on_dim_spin_activate (GtkRange     *range,
		      gpointer         user_data);

void
on_dim_spin_changed (GtkRange     *range,
		     gpointer         user_data);
			 
gboolean
on_dim_scale_focus_out_event            (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

void
on_sleep_cpu_spin_activate (GtkEditable     *editable,
			    gpointer         user_data);

void
on_sleep_cpu_spin_changed (GtkEditable     *editable,
			   gpointer         user_data);

void
irq_choose_but (GtkButton       *button,
		gpointer         user_data);

void
on_sleep_apm_toggled (GtkToggleButton *togglebutton,
		      gpointer         user_data);

void
on_sleep_probe_irq_toggled (GtkToggleButton *togglebutton,
			    gpointer         user_data);

void
irq_done_clicked (GtkButton       *button,
		  gpointer         user_data);
void
irq_select_row (GtkCList *clist, gint row, gint column,
		GdkEvent *event, gpointer user_data);

gboolean
on_sleep_idle_spin_focus_out_event     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

gboolean
on_dim_spin_focus_out_event            (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);
gboolean
on_sleep_cpu_spin_focus_out_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data);

#endif
