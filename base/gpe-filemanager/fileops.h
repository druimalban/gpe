#ifndef _FILEOPS_H
#define _FILEOPS_H

#ifdef __cplusplus
extern "C"
{
#endif

void copy_file_clip (GtkWidget *active_view, gint col);
void paste_file_clip (void);
void ask_delete_file (GtkWidget *active_view, gint col);
void ask_move_file (GtkWidget *active_view, gint col);

extern GList *file_clipboard;

#ifdef __cplusplus
}
#endif

#endif /* _FILEOPS_H */
