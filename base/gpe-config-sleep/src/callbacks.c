#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#ifdef READ_BACKLIGHT
#  include <stdio.h>
#  include <string.h>
#  include <fcntl.h>
#  include <linux/ioctl.h>
#  include <linux/h3600_ts.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "conf.h"
#include "confGUI.h"

static int runProg(char *cmd)
{
  int	status;
  pid_t	pid;

  char	*c, *argv[5];
  int	argc = 0;
  while(cmd && (*cmd != (char)NULL)) {
    if((c = strchr(cmd, ' ')) != NULL) *c++ = (char)NULL;
    argv[argc++] = cmd; cmd = c;
  }
  argv[argc++] = NULL;

  if((pid = fork()) < 0) {
    perror("fork");
    return(1);
  }
  if(pid == 0) {
    execvp(*argv, argv);
    perror(*argv);
    return(2);
  }
  while(wait(&status) != pid) /* do nothing */;

  return status;
}

void
on_sleep_idle_spin_activate (GtkEditable     *editable,
			     gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = lookup_widget((GtkWidget *)editable, "sleep_idle_spin");
  setConfigInt(ISconf, "auto-sleep_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
on_sleep_idle_spin_changed (GtkEditable     *editable,
			    gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = lookup_widget((GtkWidget *)editable, "sleep_idle_spin");
  setConfigInt(ISconf, "auto-sleep_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
AS_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data)
{
  GtkWidget	*sleepSpin;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  sleepSpin = lookup_widget((GtkWidget *)togglebutton, "sleep_idle_spin");
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
  wgt = lookup_widget((GtkWidget *)editable, "dim_spin");
  setConfigInt(ISconf, "dim_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
on_dim_spin_changed (GtkEditable     *editable,
		     gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = lookup_widget((GtkWidget *)editable, "dim_spin");
  setConfigInt(ISconf, "dim_time", gtk_spin_button_get_value_as_int((GtkSpinButton *)wgt));
}


void
AD_checked (GtkToggleButton *togglebutton,
	    gpointer         user_data)
{
  GtkWidget	*dimSpin;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  dimSpin = lookup_widget((GtkWidget *)togglebutton, "dim_spin");
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
  cpuSpin = lookup_widget((GtkWidget *)togglebutton, "sleep_cpu_spin");
  setConfigInt(ISconf, "check_cpu", gtk_toggle_button_get_active(togglebutton));
  gtk_widget_set_sensitive(cpuSpin, getConfigInt(ISconf, "check_cpu"));
}


void
on_sleep_cpu_spin_activate (GtkEditable     *editable,
			    gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = lookup_widget((GtkWidget *)editable, "sleep_cpu_spin");
  setConfigDbl(ISconf, "CPU_value", gtk_spin_button_get_value_as_float((GtkSpinButton *)wgt));
}


void
on_sleep_cpu_spin_changed (GtkEditable     *editable,
			   gpointer         user_data)
{
  GtkWidget	*wgt;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  wgt = lookup_widget((GtkWidget *)editable, "sleep_cpu_spin");
  setConfigDbl(ISconf, "CPU_value", gtk_spin_button_get_value_as_float((GtkSpinButton *)wgt));
}


void
irq_choose_but (GtkButton       *button,
		gpointer         user_data)
{
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  static GtkWidget *irqWin = NULL;

  if(!irqWin) {
    irqWin = create_irq_win(ISconf);
    init_irq_list(irqWin, gtk_widget_get_parent_window(GTK_WIDGET(button)), ISconf);
  }
  gtk_widget_show_all(irqWin);
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
  GtkWidget	*irqProbe;
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  irqProbe = lookup_widget((GtkWidget *)togglebutton, "sleep_choose_irq");
  setConfigInt(ISconf, "probe_IRQs", gtk_toggle_button_get_active(togglebutton));
  gtk_widget_set_sensitive(irqProbe, getConfigInt(ISconf, "probe_IRQs"));
}


void
bl_vscale_changed (GtkWidget       *widget,
		   gpointer         user_data)
{
  int		power;
#ifdef READ_BACKLIGHT
  struct h3600_ts_backlight bl;
#else
  char 		cmd[32];
#endif
  GtkAdjustment	*adj = GTK_ADJUSTMENT(widget);

  power = (int)adj->value;
#ifdef	READ_BACKLIGHT
  bl.power = (power ? FLITE_PWR_ON : FLITE_PWR_OFF);
  bl.brightness = power;
  if ( ioctl( ISconf->blFD, TS_SET_BACKLIGHT, bl ) < 0 ) {
    perror("Unable to write backlight");
  }
#else
  if(power) {
    sprintf(cmd, "bl %d", (int)adj->value);
  }
  else {
    sprintf(cmd, "bl off");
  }
  runProg(cmd);
#endif
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
  sprintf(cmd, "%s start", ISconf->binCmd); runProg(cmd);
}


void
stop_button (GtkButton       *button,
	     gpointer         user_data)
{
  char		cmd[64];
  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  sprintf(cmd, "%s stop", ISconf->binCmd); runProg(cmd);
}


void
mainquit (GtkButton       *button,
	  gpointer         user_data)
{
  /* do I really need to free everything */
  gtk_main_quit();
}


void
irq_done_clicked (GtkButton       *button,
		  gpointer         user_data)
{
  gtk_widget_hide(gtk_widget_get_toplevel(GTK_WIDGET(button)));
}

void
irq_select_row (GtkCList *clist, gint row, gint column,
		GdkEvent *event, gpointer user_data)
{
  int	j, found;
  int	nIrq, *irqL;

  ipaq_conf_t	*ISconf = (ipaq_conf_t *)user_data;
  if(column != 0) return;

  irqL = getConfigIntL(ISconf, &nIrq, "IRQ"); found = FALSE;
  for(j = 0; j < nIrq; j++)
    if(ISconf->ilist[row].num == irqL[j]) { found = TRUE; break; }
  free(irqL);

  if(found) {	/* turn off */
    gtk_clist_set_pixmap(clist, row, 0, get_box_pixmap(), get_box_bitmap());
    delConfigInt(ISconf, "IRQ", ISconf->ilist[row].num);
  }
  else {
    gtk_clist_set_pixmap(clist, row, 0, get_tick_pixmap(), get_tick_bitmap());
    addConfigInt(ISconf, "IRQ", ISconf->ilist[row].num);
  }
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
on_sleep_cpu_spin_focus_out_event      (GtkWidget       *widget,
                                        GdkEventFocus   *event,
                                        gpointer         user_data)
{
  on_sleep_cpu_spin_changed(GTK_EDITABLE(widget), user_data);
  return FALSE;
}
