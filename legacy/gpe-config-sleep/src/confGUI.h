#ifndef _CONFGUI_H
#define _CONFGUI_H

void init_irq_list(GtkWidget *top, GdkWindow *win, ipaq_conf_t *conf);
void set_conf_defaults(GtkWidget *top, ipaq_conf_t *conf);

GdkPixmap *get_tick_pixmap(void);
GdkBitmap *get_tick_bitmap(void);
GdkPixmap *get_box_pixmap(void);
GdkBitmap *get_box_bitmap(void);


#endif
