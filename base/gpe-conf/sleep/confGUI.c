#include <stdlib.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "conf.h"
#include "confGUI.h"

extern  GtkWidget	*sleep_enable;
extern  GtkWidget	*sleep_idle_spin;
extern  GtkWidget	*dim_enable;
extern  GtkWidget	*dim_spin;
extern  GtkWidget	*sleep_apm;
extern  GtkWidget	*sleep_cpu;
extern  GtkWidget	*sleep_cpu_spin;
extern  GtkWidget	*sleep_probe_irq;
extern  GtkWidget	*sleep_choose_irq;
extern  GtkWidget	*irqList;

void set_conf_defaults(GtkWidget *top, ipaq_conf_t *conf)
{
  int		ival;
  GtkWidget	*wgt;

  ival = getConfigInt(conf, "auto-sleep_time");
  wgt = sleep_enable;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), (ival > 0));
  wgt = sleep_idle_spin;
  gtk_spin_button_set_value((GtkSpinButton *)wgt, (gfloat)ival);
  gtk_widget_set_sensitive(wgt, (ival > 0));

  ival = getConfigInt(conf, "dim_time");
  wgt = dim_enable;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wgt), (ival > 0));
  wgt = dim_spin;
  gtk_spin_button_set_value((GtkSpinButton *)wgt, (gfloat)ival);
  gtk_widget_set_sensitive(wgt, (ival > 0));

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
  wgt = sleep_choose_irq;
  gtk_widget_set_sensitive(wgt, ival);

  /* can we get the current dim-level??? */

}

/* lifted from gpe-todo */
/*#include "tick.xpm"
#include "box.xpm"
*/
static GdkPixmap *tick_pixmap, *box_pixmap;
static GdkBitmap *tick_bitmap, *box_bitmap;

GdkPixmap *get_tick_pixmap(void) { return tick_pixmap; }
GdkPixmap *get_box_pixmap(void) { return box_pixmap; }
GdkBitmap *get_tick_bitmap(void) { return tick_bitmap; }
GdkBitmap *get_box_bitmap(void) { return box_bitmap; }

void init_irq_list(GtkWidget *top, GdkWindow *win, ipaq_conf_t *conf)
{
  int i, j, found;
  int nIrq, *irqL;
  GtkWidget *wgt;
  char *temp[2];

  if(!tick_pixmap) {
    tick_pixmap = gdk_pixmap_create_from_xpm_d (win, &tick_bitmap, NULL, tick_xpm);
    box_pixmap = gdk_pixmap_create_from_xpm_d (win, &box_bitmap, NULL, box_xpm);
  }

  wgt = irqList;
  temp[0] = ""; temp[1] = "";
  irqL = getConfigIntL(conf, &nIrq, "IRQ");
  for(i = 0; i < conf->nIrq; i++) {
    found = FALSE;
    for(j = 0; j < nIrq; j++)
      if(conf->ilist[i].num == irqL[j]) { found = TRUE; break; }
    gtk_clist_append(GTK_CLIST(wgt), temp);
#if 0
    if(found)	gtk_clist_set_pixmap(GTK_CLIST(wgt), i, 0, tick_pixmap, tick_bitmap);
    else	gtk_clist_set_pixmap(GTK_CLIST(wgt), i, 0, box_pixmap, box_bitmap);
#else
    if(found)	gtk_clist_set_text(GTK_CLIST(wgt), i, 0, "Yes");
    else	gtk_clist_set_text(GTK_CLIST(wgt), i, 0, "No");
#endif

    gtk_clist_set_text(GTK_CLIST(wgt), i, 1, conf->ilist[i].desc);
    gtk_clist_set_row_data(GTK_CLIST(wgt), i, conf);
    gtk_clist_set_selectable(GTK_CLIST(wgt), i, FALSE);
  }
  free(irqL);
}
