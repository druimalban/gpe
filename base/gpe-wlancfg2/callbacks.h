/*

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Library General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
 
#include <gtk/gtk.h>


gboolean
on_GPE_WLANCFG_de_event                (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_GPE_WLANCFG_show                    (GtkWidget       *widget,
                                        gpointer         user_data);

/*
void
on_btnUp_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_btnDown_clicked                     (GtkButton       *button,
                                        gpointer         user_data);
*/

void
on_btnDelete_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_btnNew_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_btnEdit_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_btnHelp_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_btnExpert_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_tvSchemeList_row_activated          (GtkTreeView     *treeview,
                                        GtkTreePath     *arg1,
                                        GtkTreeViewColumn *arg2,
                                        gpointer         user_data);
					
void
on_rbFrequency_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rbChannel_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cbDefault_simple_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);
