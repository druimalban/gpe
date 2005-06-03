#ifndef _GPE_CONTACTS_MAIN_H
#define _GPE_CONTACTS_MAIN_H

extern guint menu_uid;
extern void update_display (void);
extern GtkWidget *mainw;

extern gboolean mode_landscape;
extern gboolean mode_large_screen;

#ifdef IS_HILDON
typedef struct {
	gchar *title;
	gchar *list;
}t_filter;

extern t_filter *filters;
extern int current_filter;

#define IMG_HEIGHT 160
#define IMG_WIDTH  120

#endif

#endif
