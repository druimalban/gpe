#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "conf.h"
#include "confGUI.h"
#include "../applets.h"
#include "../ipaqscreen/brightness.h"

extern GtkWidget *sleep_idle_spin;
extern GtkWidget *dim_spin;
extern GtkWidget *sleep_cpu_spin;


void
on_sleep_idle_spin_activate (GtkEditable     *editable,
			     gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = sleep_idle_spin;
  setConfigInt(ISconf, "auto-sleep_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
on_sleep_idle_spin_changed (GtkEditable     *editable,
			    gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = sleep_idle_spin;
  setConfigInt(ISconf, "auto-sleep_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
AS_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data)
{
  GtkWidget	*sleepSpin;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  sleepSpin = sleep_idle_spin;;
  if(!gtk_toggle_button_get_active(togglebutton)) {
    setConfigInt(ISconf, "auto-sleep_time", 0); gtk_spin_button_set_value((GtkSpinButton *)sleepSpin, 0);
  }
  else setConfigInt(ISconf, "auto-sleep_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)sleepSpin));
  gtk_widget_set_sensitive(sleepSpin, gtk_toggle_button_get_active(togglebutton));
}

void
on_dim_spin_activate (GtkEditable     *editable,
		      gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = dim_spin;
  setConfigInt(ISconf, "dim_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
on_dim_spin_changed (GtkEditable     *editable,
		     gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = dim_spin;
  setConfigInt(ISconf, "dim_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
AD_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data)
{
  GtkWidget	*dimSpin;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  dimSpin = dim_spin;
  if(!gtk_toggle_button_get_active(togglebutton)) {
    setConfigInt(ISconf, "dim_time", 0); gtk_spin_button_set_value((GtkSpinButton *)dimSpin, 0);
  }
  else setConfigInt(ISconf, "dim_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)dimSpin));
  gtk_widget_set_sensitive(dimSpin, gtk_toggle_button_get_active(togglebutton));

}

void
cpu_checked (GtkToggleButton *togglebutton,
	     gpointer         user_data)
{
  GtkWidget	*cpuSpin;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  cpuSpin = sleep_cpu_spin;
  setConfigInt(ISconf, "check_cpu", gtk_toggle_button_get_active(togglebutton));
  gtk_widget_set_sensitive(cpuSpin, getConfigInt(ISconf, "check_cpu"));
}


void
on_sleep_cpu_spin_activate (GtkEditable     *editable,
			    gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = sleep_cpu_spin;
  setConfigDbl(ISconf, "CPU_value", gtk_spin_button_get_value_as_float((GtkSpinButton *)wgt));
}


void
on_sleep_cpu_spin_changed (GtkEditable     *editable,
			   gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = sleep_cpu_spin;
  setConfigDbl(ISconf, "CPU_value", gtk_spin_button_get_value_as_float((GtkSpinButton *)wgt));
}


void
on_sleep_apm_toggled (GtkToggleButton *togglebutton,
		      gpointer         user_data)
{
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  setConfigInt(ISconf, "check_apm", gtk_toggle_button_get_active(togglebutton));
}


void
on_sleep_probe_irq_toggled (GtkToggleButton *togglebutton,
			    gpointer         user_data)
{
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  setConfigInt(ISconf, "probe_IRQs", gtk_toggle_button_get_active(togglebutton));
}


void
bl_vscale_changed (GtkWidget       *widget,
		   gpointer         user_data)
{
  int		power;
  char 		val[32];
  GtkAdjustment	*adj = GTK_ADJUSTMENT(widget);

  power = (int)adj->value;
  snprintf(val,32,"%d",power);
  suid_exec("SCRB",val);
}


void
start_button (GtkButton       *button,
	      gpointer         user_data)
{
  char		cmd[64];
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  sprintf(cmd, "%s stop", ISconf->binCmd); runProg(cmd);
  if(save_ISconf(ISconf, ISconf->confName)) {
    char homeConf[MAXPATHLEN];
    sprintf(homeConf, "%s/ipaq-sleep.conf", getenv("HOME"));
    if(!save_ISconf(ISconf, homeConf))
      strcpy(ISconf->confName, homeConf);
  }
  sprintf(cmd, "%s start", ISconf->binCmd);
  runProg(cmd);
}


void
stop_button (GtkButton       *button,
	     gpointer         user_data)
{
  char		cmd[64];
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  sprintf(cmd, "%s stop", ISconf->binCmd); 
  runProg(cmd);
}


gboolean
on_sleep_idle_spin_focus_out_event     (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  on_sleep_idle_spin_changed(GTK_EDITABLE(widget), user_data);
  return FALSE;
}


gboolean
on_dim_spin_focus_out_event            (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  on_dim_spin_changed(GTK_EDITABLE(widget), user_data);
  return FALSE;
}

gboolean
on_dim_scale_focus_out_event            (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  set_brightness((int)user_data);
  return FALSE;
}

gboolean
on_sleep_cpu_spin_focus_out_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  on_sleep_cpu_spin_changed(GTK_EDITABLE(widget), user_data);
  return FALSE;
}
