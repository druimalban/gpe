#ifndef _PROTO_H
#define _PROTO_H

extern void update_display (void);
extern void configure (GtkWidget * widget, gpointer d);
extern void edit_person (struct person *p);
extern void load_panel_config ();
void update_edit (struct person *p, GtkWidget *w);

#endif
