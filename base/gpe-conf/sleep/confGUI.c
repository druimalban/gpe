#include <stdlib.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "conf.h"
#include "confGUI.h"

extern  GtkWidget	*sleep_enable;
extern  GtkWidget	*sleep_idle_spin;
extern  GtkWidget	*dim_enable;
extern  GtkWidget	*dim_spin;
extern  GtkWidget	*dim_scale;
extern  GtkWidget	*sleep_apm;
extern  GtkWidget	*sleep_cpu;
extern  GtkWidget	*sleep_cpu_spin;
extern  GtkWidget	*sleep_probe_irq;


void set_conf_defaults(GtkWidget *top, ipaq_conf_t *conf)
{
  int		ival;
  GtkWidget	*wgt;

  ival = getConfigInt(conf, "auto-sleep_time");
  wgt = sleep_enable;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), (ival > 0));
  wgt = sleep_idle_spin;
  gtk_range_set_value(GTK_RANGE(wgt), (gfloat)ival);
  gtk_widget_set_sensitive(wgt, (ival > 0));

  ival = getConfigInt(conf, "dim_time");
  wgt = dim_enable;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), (ival > 0));
  wgt = dim_spin;
  gtk_range_set_value(GTK_RANGE(wgt), (gfloat)ival);
  gtk_widget_set_sensitive(wgt, (ival > 0));
  gtk_widget_set_sensitive(dim_scale, (ival > 0));

  ival = getConfigInt(conf, "dim_level");
  wgt = dim_scale;
  gtk_range_set_value(GTK_RANGE(wgt), (gfloat)ival);
  
  wgt = sleep_apm;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), getConfigInt(conf, "check_apm"));  

  ival = getConfigInt(conf, "check_cpu");
  wgt = sleep_cpu;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), ival);
  wgt = sleep_cpu_spin;
  gtk_spin_button_set_value((GtkSpinButton *)wgt, getConfigDbl(conf, "CPU_value"));
  gtk_widget_set_sensitive(wgt, ival);

  ival = getConfigInt(conf, "probe_IRQs");
  wgt = sleep_probe_irq;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), ival);
}
