#include <gtk/gtk.h>

enum
{
  COLUMN_USER,
  COLUMN_READ,
  COLUMN_WRITE,
  NUM_COLUMNS
};

void  list_toggle_read (GtkCellRendererToggle *cellrenderertoggle,
                                            gchar *path_str,
                                            gpointer model_data);

void  list_toggle_write (GtkCellRendererToggle *cellrenderertoggle,
                                            gchar *path_str,
                                            gpointer model_data);

gboolean
on_wdcmain_destroy_event               (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_wdcmain_show                        (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_wdcmain_destroy                     (GtkObject       *object,
                                        gpointer         user_data);

void
on_combo_entry2_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_combo_entry1_changed                (GtkEditable     *editable,
                                        gpointer         user_data);
void
on_bNew_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_bDelete_clicked (GtkButton * button, gpointer user_data);

void
on_eBaseDir_editing_done               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_ePolicy_editing_done                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_eUserdir_changed                    (GtkEditable     *editable,
                                        gpointer         user_data);
