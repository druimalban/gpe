#ifndef _PROTO_H
#define _PROTO_H

extern void update_display (void);
extern void configure (GtkWidget * widget, gpointer d);
extern void edit_person (struct person *p, gboolean isnew);
extern void load_panel_config ();
extern void update_edit (struct person *p, GtkWidget *w);
extern gchar *build_categories_string (struct person *p);

#endif
