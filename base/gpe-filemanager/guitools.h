#ifndef _GUITOOLS_H
#define _GUITOOLS_H

#ifdef __cplusplus
extern "C"
{
#endif

GtkWidget *build_storage_menu(gboolean wide);
void set_active_item(char* path);
gboolean do_scheduled_update(void);

#ifdef __cplusplus
}
#endif

#endif /* _GUITOOLS_H */
