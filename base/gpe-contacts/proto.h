#ifndef _PROTO_H
#define _PROTO_H

extern void update_display (void);
extern void configure (GtkWidget * widget, gpointer d);
extern void edit_person (struct contacts_person *p, gchar *title, gboolean isdialog);
extern void load_panel_config ();
extern void update_edit (struct contacts_person *p, GtkWidget *w);
extern gchar *build_categories_string (struct contacts_person *p);

#endif
