#include <gtk/gtk.h>


void
on_smallphotobutton_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_bigphotobutton_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_bigphotobutton_size_allocate        (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data);

void
on_smallphotobutton_size_allocate      (GtkWidget       *widget,
                                        GtkAllocation   *allocation,
                                        gpointer         user_data);
