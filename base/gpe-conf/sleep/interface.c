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
#include <gpe/spacing.h>

#include "callbacks.h"
#include "interface.h"
#include "../applets.h"
#include "../ipaqscreen/brightness.h"

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
  GtkObject *sleep_idle_spin_adj;
  GtkWidget *sleep_idle_label;
  GtkWidget *dim;
  GtkObject *dim_spin_adj;
  GtkWidget *dim_idle_label;
  GtkWidget *frame5;
  GtkObject *sleep_cpu_spin_adj;
  GtkWidget *sleep_cpu_label;
  GtkWidget *frame6;
  GtkWidget *dim_scale;
  GtkObject *dim_adj;
  GtkTooltips *tooltips;

  char *tstr;
  int gpe_border = gpe_get_border();
  int gpe_box_spacing = gpe_get_boxspacing();

  int blVal;
  
  blVal = get_brightness();
  
  tooltips = gtk_tooltips_new ();

  GPE_Config_Sleep = gtk_hbox_new (0,0);

  table1 = gtk_table_new (4, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table1), gpe_border);

  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (GPE_Config_Sleep), table1);

  sleep = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Auto sleep"));
  gtk_label_set_markup (GTK_LABEL (sleep), tstr);
  g_free (tstr);
  
  gtk_widget_show (sleep);
  gtk_misc_set_alignment (GTK_MISC (sleep), 0, 0.0);
  gtk_table_attach (GTK_TABLE (table1), sleep, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0 , 0);

  sleep_enable = gtk_check_button_new_with_label (_("enabled"));
  gtk_widget_show (sleep_enable);
  gtk_table_attach (GTK_TABLE (table1), sleep_enable, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
  
  sleep_idle_spin_adj = gtk_adjustment_new (180, 0, 1800, 5, 10, 10);
  sleep_idle_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sleep_idle_spin_adj), 1, 0);
  gtk_widget_show (sleep_idle_spin);
  
  gtk_table_attach (GTK_TABLE (table1), sleep_idle_spin, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, gpe_box_spacing);

  gtk_tooltips_set_tip (tooltips, sleep_idle_spin, _("sleep timeout"), NULL);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sleep_idle_spin), TRUE);

  sleep_idle_label = gtk_label_new (_("idle(sec)"));
  gtk_widget_show (sleep_idle_label);
  gtk_table_attach (GTK_TABLE (table1), sleep_idle_label, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, gpe_box_spacing);
  gtk_misc_set_alignment (GTK_MISC (sleep_idle_label), 0, 0.5);


  dim = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Auto dim"));
  gtk_label_set_markup (GTK_LABEL (dim), tstr);
  g_free (tstr);
  gtk_widget_show (dim);
  gtk_misc_set_alignment (GTK_MISC (dim), 0, 0.0);
  
  gtk_table_attach (GTK_TABLE (table1), dim, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);
					
  dim_enable = gtk_check_button_new_with_label (_("enabled"));
  gtk_widget_show (dim_enable);
  gtk_table_attach (GTK_TABLE (table1), dim_enable, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
  
  dim_spin_adj = gtk_adjustment_new (60, 0, 1800, 5, 10, 10);
  dim_spin = gtk_spin_button_new (GTK_ADJUSTMENT (dim_spin_adj), 1, 0);
  gtk_widget_show (dim_spin);
  gtk_table_attach (GTK_TABLE (table1), dim_spin, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0, gpe_box_spacing);
					
  gtk_tooltips_set_tip (tooltips, dim_spin, _("backlight dim timeout"), NULL);

  dim_idle_label = gtk_label_new (_("idle(sec)"));
  gtk_widget_show (dim_idle_label);
  gtk_table_attach (GTK_TABLE (table1), dim_idle_label, 2, 3, 3, 4,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, gpe_box_spacing);
  gtk_misc_set_alignment (GTK_MISC (dim_idle_label), 0, 0.5);

  frame6 = gtk_label_new (_("Auto dim level"));
  gtk_widget_show (frame6);
  gtk_table_attach (GTK_TABLE (table1), frame6, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);
  gtk_container_set_border_width (GTK_CONTAINER (frame6), 1);

  dim_adj = gtk_adjustment_new (blVal, 0, 255, 1, 5, 0);
  dim_scale = gtk_hscale_new (GTK_ADJUSTMENT (dim_adj));

  gtk_widget_show (dim_scale);
  gtk_scale_set_digits (GTK_SCALE (dim_scale), 0);
  gtk_table_attach (GTK_TABLE (table1),dim_scale, 1, 4, 5, 6,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL), 0, 0);


  frame5 = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Auto sleep advanced controls"));
  gtk_label_set_markup (GTK_LABEL (frame5), tstr);
  g_free (tstr);
  gtk_misc_set_alignment (GTK_MISC (frame5), 0, 0.0);

  gtk_widget_show (frame5);
  gtk_table_attach (GTK_TABLE (table1), frame5, 0, 3, 6, 7,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, gpe_box_spacing);
					
  sleep_cpu = gtk_check_button_new_with_label (_("CPU"));

  gtk_widget_show (sleep_cpu);
  gtk_table_attach (GTK_TABLE (table1), sleep_cpu, 0, 1, 7, 8,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
  
  gtk_tooltips_set_tip (tooltips, sleep_cpu, _("Sleep on cpu less than load"), NULL);

  sleep_cpu_spin_adj = gtk_adjustment_new (0.05, 0.01, 1.2, 0.01, 0.05, 10);
  sleep_cpu_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sleep_cpu_spin_adj), 1, 2);
  gtk_widget_show (sleep_cpu_spin);
  gtk_table_attach (GTK_TABLE (table1), sleep_cpu_spin, 1, 2, 7, 8,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), 0.99, 0.0);
					
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sleep_cpu_spin), TRUE);

  sleep_cpu_label = gtk_label_new (_("load"));
  gtk_widget_show (sleep_cpu_label);
  gtk_table_attach (GTK_TABLE (table1), sleep_cpu_label, 2, 3, 7, 8,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
  gtk_misc_set_alignment (GTK_MISC (sleep_cpu_label), 0, 0.0);
  
  sleep_apm = gtk_check_button_new_with_label (_("APM"));
  gtk_widget_show (sleep_apm);
  gtk_table_attach (GTK_TABLE (table1), sleep_apm, 0, 1, 8, 9,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
  
  gtk_tooltips_set_tip (tooltips, sleep_apm, _("Sleep on AC"), NULL);

  sleep_probe_irq = gtk_check_button_new_with_label (_("probe IRQs"));
  gtk_widget_show (sleep_probe_irq);
  gtk_table_attach (GTK_TABLE (table1), sleep_probe_irq, 0, 2, 9, 10,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);

  gtk_tooltips_set_tip (tooltips, sleep_probe_irq, _("Check IRQ activity"), NULL);

  sleep_choose_irq = gtk_button_new_with_label (_("Choose IRQs"));
  gtk_widget_show (sleep_choose_irq);
  gtk_table_attach (GTK_TABLE (table1), sleep_choose_irq, 2, 4, 9, 10,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK), gpe_box_spacing, 0);
					
  gtk_container_set_border_width (GTK_CONTAINER (sleep_choose_irq), 1);
  
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
  gtk_signal_connect (GTK_OBJECT (dim_scale), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_dim_scale_focus_out_event),
                      (gpointer)blVal);
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
