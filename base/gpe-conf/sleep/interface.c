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
GtkWidget *sleep_enable;
GtkWidget *dim_enable;
GtkWidget *sleep_apm;
GtkWidget *sleep_cpu;
GtkWidget *sleep_probe_irq;

extern GtkStyle *wstyle;
GtkWidget*
create_GPE_Config_Sleep (ipaq_conf_t *ISconf)
{
  GtkWidget *GPE_Config_Sleep;
  GtkWidget *table1, *hbox;
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

  GPE_Config_Sleep = table1 = gtk_table_new (4, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table1), gpe_border);

  gtk_widget_show (table1);

  sleep = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Auto sleep"));
  gtk_label_set_markup (GTK_LABEL (sleep), tstr);
  g_free (tstr);
  
  gtk_widget_show (sleep);
  gtk_misc_set_alignment (GTK_MISC (sleep), 0, 0.5);
  gtk_table_attach (GTK_TABLE (table1), sleep, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0 , 0);

  sleep_enable = gtk_check_button_new_with_label (_("enabled"));
  gtk_widget_show (sleep_enable);
  gtk_table_attach (GTK_TABLE (table1), sleep_enable, 0, 1, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), gpe_box_spacing, 0);
  
  sleep_idle_spin_adj = gtk_adjustment_new (180, 0, 1800, 5, 10, 10);
  sleep_idle_spin = gtk_hscale_new(GTK_ADJUSTMENT (sleep_idle_spin_adj));
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "format-value",
                      GTK_SIGNAL_FUNC (change_scale_label),
                      NULL);

  gtk_widget_show (sleep_idle_spin);
  
  gtk_table_attach (GTK_TABLE (table1), sleep_idle_spin, 1, 3, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, gpe_box_spacing);

  gtk_tooltips_set_tip (tooltips, sleep_idle_spin, _("sleep timeout"), NULL);

 /* sleep_idle_label = gtk_label_new (_("Delay:"));
  gtk_misc_set_alignment (GTK_MISC(sleep_idle_label), 0.0, 0.5);
  gtk_widget_show (sleep_idle_label);
  gtk_table_attach (GTK_TABLE (table1), sleep_idle_label, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), gpe_box_spacing, gpe_box_spacing);
*/

  dim = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Auto dim"));
  gtk_label_set_markup (GTK_LABEL (dim), tstr);
  g_free (tstr);
  gtk_widget_show (dim);
  gtk_misc_set_alignment (GTK_MISC (dim), 0, 0.5);
  
  gtk_table_attach (GTK_TABLE (table1), dim, 0, 3, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
					
  dim_enable = gtk_check_button_new_with_label (_("enabled"));
  gtk_widget_show (dim_enable);
  gtk_table_attach (GTK_TABLE (table1), dim_enable, 0, 1, 3, 4,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), gpe_box_spacing, 0);
  
  dim_spin_adj = gtk_adjustment_new (60, 0, 1800, 5, 10, 10);
  dim_spin = gtk_hscale_new (GTK_ADJUSTMENT (dim_spin_adj));
  gtk_signal_connect (GTK_OBJECT (dim_spin), "format-value",
                      GTK_SIGNAL_FUNC (change_scale_label),
                      NULL);

  gtk_widget_show (dim_spin);
  gtk_table_attach (GTK_TABLE (table1), dim_spin, 1, 3, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, gpe_box_spacing);
					
  gtk_tooltips_set_tip (tooltips, dim_spin, _("backlight dim timeout"), NULL);

/*  dim_idle_label = gtk_label_new (_("Delay:"));
  gtk_misc_set_alignment (GTK_MISC(dim_idle_label), 0.0, 0.5);
  gtk_widget_show (dim_idle_label);
  gtk_table_attach (GTK_TABLE (table1), dim_idle_label, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), gpe_box_spacing, gpe_box_spacing);
*/
  frame6 = gtk_label_new (_("Auto dim level"));
  gtk_misc_set_alignment (GTK_MISC(frame6), 0.0, 0.5);
  gtk_widget_show (frame6);
  gtk_table_attach (GTK_TABLE (table1), frame6, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  dim_adj = gtk_adjustment_new (blVal, 0, 255, 1, 5, 0);
  dim_scale = gtk_hscale_new (GTK_ADJUSTMENT (dim_adj));

  gtk_widget_show (dim_scale);
  gtk_scale_set_digits (GTK_SCALE (dim_scale), 0);
  gtk_table_attach (GTK_TABLE (table1),dim_scale, 1, 3, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);


  frame5 = gtk_label_new (NULL);
  tstr = g_strdup_printf ("<b>%s</b>", _("Advanced Controls"));
  gtk_label_set_markup (GTK_LABEL (frame5), tstr);
  g_free (tstr);
  gtk_misc_set_alignment (GTK_MISC (frame5), 0, 0.5);

  gtk_widget_show (frame5);
  gtk_table_attach (GTK_TABLE (table1), frame5, 0, 3, 6, 7,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, gpe_box_spacing);
					
  sleep_cpu = gtk_check_button_new_with_label (_("CPU"));
  gtk_widget_show (sleep_cpu);
  gtk_table_attach (GTK_TABLE (table1), sleep_cpu, 0, 1, 7, 8,
                    GTK_FILL,
                    GTK_FILL, gpe_box_spacing, 0);
  
  gtk_tooltips_set_tip (tooltips, sleep_cpu, _("Sleep on cpu less than load"), NULL);

      hbox = gtk_hbox_new(FALSE, gpe_box_spacing);
  
	  sleep_cpu_label = gtk_label_new (_("load below"));
	  gtk_widget_show (sleep_cpu_label);
	  gtk_misc_set_alignment (GTK_MISC (sleep_cpu_label), 0, 0.5);
	  gtk_box_pack_start(GTK_BOX(hbox), sleep_cpu_label, FALSE, TRUE, 0);
	  
	  sleep_cpu_spin_adj = gtk_adjustment_new (0.05, 0.01, 1.2, 0.01, 0.05, 10);
	  sleep_cpu_spin = gtk_spin_button_new (GTK_ADJUSTMENT (sleep_cpu_spin_adj), 1, 2);
	  gtk_widget_show (sleep_cpu_spin);
	  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (sleep_cpu_spin), TRUE);
	  gtk_box_pack_start(GTK_BOX(hbox), sleep_cpu_spin, FALSE, TRUE, 0);
	
	  gtk_table_attach (GTK_TABLE (table1), hbox, 1, 3, 7, 8,
						(GtkAttachOptions) (GTK_FILL),
						(GtkAttachOptions) (GTK_FILL), gpe_box_spacing, 0);
  
  sleep_apm = gtk_check_button_new_with_label (_("APM"));
  gtk_widget_show (sleep_apm);
  gtk_table_attach (GTK_TABLE (table1), sleep_apm, 0, 1, 8, 9,
                    GTK_FILL,
                    GTK_FILL, gpe_box_spacing, 0);
  
  gtk_tooltips_set_tip (tooltips, sleep_apm, _("Sleep on AC"), NULL);

  sleep_probe_irq = gtk_check_button_new_with_label (_("probe IRQs"));
  gtk_widget_show (sleep_probe_irq);
  gtk_table_attach (GTK_TABLE (table1), sleep_probe_irq, 0, 1, 9, 10,
                    GTK_FILL,
                    GTK_FILL, gpe_box_spacing, 0);

  gtk_tooltips_set_tip (tooltips, sleep_probe_irq, _("Check IRQ activity"), NULL);

  gtk_signal_connect (GTK_OBJECT (GPE_Config_Sleep), "delete_event",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "value-changed",
                      GTK_SIGNAL_FUNC (on_sleep_idle_spin_changed),
                      ISconf);
					  
  gtk_signal_connect (GTK_OBJECT (sleep_idle_spin), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_sleep_idle_spin_focus_out_event),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_enable), "toggled",
                      GTK_SIGNAL_FUNC (AS_checked),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (dim_spin), "value-changed",
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
  gtk_signal_connect (GTK_OBJECT (sleep_cpu_spin), "changed",
                      GTK_SIGNAL_FUNC (on_sleep_cpu_spin_changed),
                      ISconf);
  gtk_signal_connect (GTK_OBJECT (sleep_cpu_spin), "focus_out_event",
                      GTK_SIGNAL_FUNC (on_sleep_cpu_spin_focus_out_event),
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
