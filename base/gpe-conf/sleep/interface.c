#ifdef READ_BACKLIGHT
#  include <stdio.h>
#  include <string.h>
#  include <fcntl.h>
#  include <linux/ioctl.h>
#  include <linux/h3600_ts.h>
#  define DEV_NODE "/dev/touchscreen/0"
#endif

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <libintl.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gpe/picturebutton.h>

#include "callbacks.h"
#include "interface.h"
#include "../applets.h"

GtkWidget *sleep_idle_spin;
GtkWidget *dim_spin;
GtkWidget *sleep_cpu_spin;
GtkWidget *sleep_choose_irq;
GtkWidget *sleep_enable;
GtkWidget *dim_enable;
GtkWidget *sleep_apm;
GtkWidget *sleep_cpu;
GtkWidget *sleep_probe_irq;
GtkWidget *irqList;

extern GtkStyle *wstyle;
GtkWidget*
create_GPE_Config_Sleep (ipaq_conf_t *ISconf)
{
  GtkWidget *GPE_Config_Sleep;
  GtkWidget *table1;
  GtkWidget *sleep;
  GtkWidget *table2;
  GtkObject *sleep_idle_spin_adj;
  GtkWidget *sleep_idle_label;
  GtkWidget *dim;
  GtkWidget *table3;
  GtkObject *dim_spin_adj;
  GtkWidget *dim_idle_label;
  GtkWidget *frame5;
  GtkWidget *table4;
  GtkObject *sleep_cpu_spin_adj;
  GtkWidget *sleep_cpu_label;
  GtkWidget *frame6;
  GtkWidget *vbox2;
  GtkWidget *dim_scale;
  GtkObject *dim_adj;
  GtkWidget *hbuttonbox1;
  GtkWidget *start_but;
  GtkWidget *stop_but;
  GtkTooltips *tooltips;

  int blVal;
#ifdef READ_BACKLIGHT
  struct h3600_ts_backlight bl;
#endif

  tooltips = gtk_tooltips_new ();

  GPE_Config_Sleep = gtk_hbox_new (0,0);

  table1 = gtk_table_new (3, 2, FALSE);

  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (GPE_Config_Sleep), table1);

  sleep = gtk_frame_new ("Auto-sleep");

  gtk_widget_show (sleep);
  gtk_table_attach (GTK_TABLE (table1), sleep, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (sleep), 1);

  table2 = gtk_table_new (2, 2, FALSE);

  gtk_widget_show (table2);
  gtk_container_add (GTK_CONTAINER (sleep), table2);

  sleep_idle_spin_adj = gtk_adjustment_new (180, 0, 1800, 5, 10, 10);
  sleep_idle_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sleep_idle_spin_adj), 1, 0);

  gtk_widget_show (sleep_idle_spin);
  gtk_table_attach (GTK_TABLE (table2), sleep_idle_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);

  gtk_tooltips_set_tip (tooltips, sleep_idle_spin, "sleep timeout", NULL);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sleep_idle_spin), TRUE);

  sleep_idle_label = gtk_label_new ("idle(sec)");

  gtk_widget_show (sleep_idle_label);
  gtk_table_attach (GTK_TABLE (table2), sleep_idle_label, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (sleep_idle_label), 0, 0.5);

  sleep_enable = gtk_check_button_new_with_label ("enabled");

  gtk_widget_show (sleep_enable);
  gtk_table_attach (GTK_TABLE (table2), sleep_enable, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (sleep_enable, GTK_CAN_FOCUS);

  dim = gtk_frame_new ("Auto-dim");

  gtk_widget_show (dim);
  gtk_table_attach (GTK_TABLE (table1), dim, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (dim), 1);

  table3 = gtk_table_new (2, 2, FALSE);

  gtk_widget_show (table3);
  gtk_container_add (GTK_CONTAINER (dim), table3);

  dim_spin_adj = gtk_adjustment_new (60, 0, 1800, 5, 10, 10);
  dim_spin = gtk_spin_button_new (GTK_ADJUSTMENT (dim_spin_adj), 1, 0);

  gtk_widget_show (dim_spin);
  gtk_table_attach (GTK_TABLE (table3), dim_spin, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_tooltips_set_tip (tooltips, dim_spin, "backlight dim timeout", NULL);

  dim_idle_label = gtk_label_new ("idle(sec)");

  gtk_widget_show (dim_idle_label);
  gtk_table_attach (GTK_TABLE (table3), dim_idle_label, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (dim_idle_label), 0, 0.5);

  dim_enable = gtk_check_button_new_with_label ("enabled");

  gtk_widget_show (dim_enable);
  gtk_table_attach (GTK_TABLE (table3), dim_enable, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (dim_enable, GTK_CAN_FOCUS);

  frame5 = gtk_frame_new ("Auto-sleep");

  gtk_widget_show (frame5);
  gtk_table_attach (GTK_TABLE (table1), frame5, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame5), 1);

  table4 = gtk_table_new (4, 3, FALSE);

  gtk_widget_show (table4);
  gtk_container_add (GTK_CONTAINER (frame5), table4);

  sleep_cpu = gtk_check_button_new_with_label ("cpu");

  gtk_widget_show (sleep_cpu);
  gtk_table_attach (GTK_TABLE (table4), sleep_cpu, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (sleep_cpu, GTK_CAN_FOCUS);
  gtk_tooltips_set_tip (tooltips, sleep_cpu, "Sleep on cpu load", NULL);

  sleep_cpu_spin_adj = gtk_adjustment_new (0.05, 0.01, 1.2, 0.01, 0.05, 10);
  sleep_cpu_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sleep_cpu_spin_adj), 1, 2);

  gtk_widget_show (sleep_cpu_spin);
  gtk_table_attach (GTK_TABLE (table4), sleep_cpu_spin, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sleep_cpu_spin), TRUE);

  sleep_cpu_label = gtk_label_new ("load");

  gtk_widget_show (sleep_cpu_label);
  gtk_table_attach (GTK_TABLE (table4), sleep_cpu_label, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (sleep_cpu_label), 0, 0.5);

  sleep_choose_irq = gtk_button_new_with_label ("Choose IRQs");

  gtk_widget_show (sleep_choose_irq);
  gtk_table_attach (GTK_TABLE (table4), sleep_choose_irq, 0, 3, 3, 4,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (sleep_choose_irq), 1);
  GTK_WIDGET_UNSET_FLAGS (sleep_choose_irq, GTK_CAN_FOCUS);

  sleep_apm = gtk_check_button_new_with_label ("apm");

  gtk_widget_show (sleep_apm);
  gtk_table_attach (GTK_TABLE (table4), sleep_apm, 0, 3, 0, 1,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (sleep_apm, GTK_CAN_FOCUS);
  gtk_tooltips_set_tip (tooltips, sleep_apm, "Sleep on AC", NULL);

  sleep_probe_irq = gtk_check_button_new_with_label ("probe IRQs");

  gtk_widget_show (sleep_probe_irq);
  gtk_table_attach (GTK_TABLE (table4), sleep_probe_irq, 0, 3, 2, 3,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (sleep_probe_irq, GTK_CAN_FOCUS);
  gtk_tooltips_set_tip (tooltips, sleep_probe_irq, "Check IRQ activity", NULL);

  frame6 = gtk_frame_new ("Auto-dim level");

  gtk_widget_show (frame6);
  gtk_table_attach (GTK_TABLE (table1), frame6, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 1);

  vbox2 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (frame6), vbox2);

  blVal = 32;
#ifdef READ_BACKLIGHT
  ISconf->blFD = open(DEV_NODE,O_RDWR);
  if( ISconf->blFD < 0 ) {
    perror(DEV_NODE);
  } 

  if ( ioctl( ISconf->blFD, TS_GET_BACKLIGHT, &bl ) < 0 ) {
    perror("Unable to read backlight");
    close(ISconf->blFD);
  } 
  blVal = bl.brightness;
#endif

  dim_adj = gtk_adjustment_new (blVal, 0, 255, 1, 5, 0);
  dim_scale = gtk_vscale_new (GTK_ADJUSTMENT (dim_adj));

  gtk_widget_show (dim_scale);
  gtk_box_pack_start (GTK_BOX (vbox2), dim_scale, TRUE, TRUE, 0);
  gtk_scale_set_digits (GTK_SCALE (dim_scale), 0);

  hbuttonbox1 = gtk_hbutton_box_new ();

  gtk_widget_show (hbuttonbox1);
  gtk_table_attach (GTK_TABLE (table1), hbuttonbox1, 0, 2, 2, 3,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 28, 0);

  start_but = gpe_picture_button(wstyle, _("Start"),"media-play" );
  
  gtk_widget_show (start_but);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), start_but);
  gtk_container_set_border_width (GTK_CONTAINER (start_but), 1);
  GTK_WIDGET_SET_FLAGS (start_but, GTK_CAN_DEFAULT);

  stop_but = gpe_picture_button(wstyle, _("Stop"),"media-stop" );//gtk_button_new_with_label ("Stop");

  gtk_widget_show (stop_but);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), stop_but);
  gtk_container_set_border_width (GTK_CONTAINER (stop_but), 1);
  GTK_WIDGET_SET_FLAGS (stop_but, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (GPE_Config_Sleep), "delete_event",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "activate",
                      GTK_SIGNAL_FUNC (on_sleep_idle_spin_activate),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "changed",
                      GTK_SIGNAL_FUNC (on_sleep_idle_spin_changed),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_sleep_idle_spin_focus_out_event),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_enable), "toggled",
                      GTK_SIGNAL_FUNC (AS_checked),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_spin), "activate",
                      GTK_SIGNAL_FUNC (on_dim_spin_activate),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_spin), "changed",
                      GTK_SIGNAL_FUNC (on_dim_spin_changed),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_spin), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_dim_spin_focus_out_event),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_enable), "toggled",
                      GTK_SIGNAL_FUNC (AD_checked),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_cpu), "toggled",
                      GTK_SIGNAL_FUNC (cpu_checked),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_cpu_spin), "activate",
                      GTK_SIGNAL_FUNC (on_sleep_cpu_spin_activate),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_cpu_spin), "changed",
                      GTK_SIGNAL_FUNC (on_sleep_cpu_spin_changed),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_cpu_spin), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_sleep_cpu_spin_focus_out_event),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_choose_irq), "clicked",
                      GTK_SIGNAL_FUNC (irq_choose_but),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_apm), "toggled",
                      GTK_SIGNAL_FUNC (on_sleep_apm_toggled),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_probe_irq), "toggled",
                      GTK_SIGNAL_FUNC (on_sleep_probe_irq_toggled),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_adj), "value_changed",
                      GTK_SIGNAL_FUNC (bl_vscale_changed),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (start_but), "clicked",
                      GTK_SIGNAL_FUNC (start_button),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (stop_but), "clicked",
                      GTK_SIGNAL_FUNC (stop_button),
                      ISconf);

  gtk_object_set_data (GTK_OBJECT (GPE_Config_Sleep), "tooltips", tooltips);

  return GPE_Config_Sleep;
}

GtkWidget*
create_irq_win (ipaq_conf_t *ISconf)
{
  GtkWidget *irq_win;
  GtkWidget *vbox3;
  GtkWidget *scrolledwindow1;
  GtkWidget *irq_tog;
  GtkWidget *irq_des;
  GtkWidget *irq_done;

#if GTK_MAJOR_VERSION >= 2
  irq_win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#else
  irq_win = gtk_window_new (GTK_WINDOW_DIALOG);
#endif
  
  gtk_window_set_title (GTK_WINDOW (irq_win), "Select IRQs");
  gtk_window_set_modal (GTK_WINDOW (irq_win), TRUE);
  gtk_window_set_default_size (GTK_WINDOW (irq_win), 200, 150);
  gtk_window_set_policy (GTK_WINDOW (irq_win), TRUE, TRUE, TRUE);

  vbox3 = gtk_vbox_new (FALSE, 0);

  gtk_widget_show (vbox3);
  gtk_container_add (GTK_CONTAINER (irq_win), vbox3);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);

  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox3), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  irqList = gtk_clist_new (2);

  gtk_widget_show (irqList);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), irqList);
  GTK_WIDGET_UNSET_FLAGS (irqList, GTK_CAN_FOCUS);
  gtk_clist_set_column_width (GTK_CLIST (irqList), 0, 34);
  gtk_clist_set_column_width (GTK_CLIST (irqList), 1, 80);
  gtk_clist_column_titles_show (GTK_CLIST (irqList));

  irq_tog = gtk_label_new ("Active");

  gtk_widget_show (irq_tog);
  gtk_clist_set_column_widget (GTK_CLIST (irqList), 0, irq_tog);
  gtk_label_set_justify (GTK_LABEL (irq_tog), GTK_JUSTIFY_LEFT);

  irq_des = gtk_label_new ("Description");

  gtk_widget_show (irq_des);
  gtk_clist_set_column_widget (GTK_CLIST (irqList), 1, irq_des);
  gtk_label_set_justify (GTK_LABEL (irq_des), GTK_JUSTIFY_LEFT);

  irq_done = gtk_button_new_with_label ("Done");

  gtk_widget_show (irq_done);
  gtk_box_pack_start (GTK_BOX (vbox3), irq_done, FALSE, FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (irq_done), 2);
  GTK_WIDGET_UNSET_FLAGS (irq_done, GTK_CAN_FOCUS);

  gtk_signal_connect (GTK_OBJECT (irq_win), "delete_event",
                      GTK_SIGNAL_FUNC (gtk_widget_hide),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (irq_win), "destroy_event",
                      GTK_SIGNAL_FUNC (gtk_widget_hide),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (irqList), "select_row",
                      GTK_SIGNAL_FUNC (irq_select_row),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (irqList), "unselect_row",
                      GTK_SIGNAL_FUNC (irq_select_row),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (irq_done), "clicked",
                      GTK_SIGNAL_FUNC (irq_done_clicked),
                      ISconf);

  return irq_win;
}

