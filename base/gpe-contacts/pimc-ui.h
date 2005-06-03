/*

 */

#ifndef PIMC_UI_H
#define PIMC_UI_H

#include <gtk/gtk.h>

GtkWidget *build_pim_categories_list (void);
void populate_pim_categories_list (GtkWidget *w, GSList * selected_categories);
GSList *get_categories (GtkWidget *w);

#endif
