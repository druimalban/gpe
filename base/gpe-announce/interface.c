/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/render.h>

#include "announce.h"

GtkWidget*
create_window (char *announcetext)
{
  GtkWidget *AlarmWin;
  GtkWidget *dialog_vbox3;
  GtkWidget *frame4;
  GtkWidget *table6;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkWidget *label38;
  GtkWidget *label37;
  GtkWidget *label36;
  GtkWidget *AlarmComment;
  GtkWidget *AlarmTime;
  GtkWidget *AlarmDate;
  GtkWidget *label32;
  GtkWidget *dialog_action_area3;
  GtkWidget *vbox6;
  GtkWidget *hbuttonbox8;
  GtkWidget *AlarmMute;
  GtkWidget *AlarmDelay;
  GtkWidget *hbuttonbox7;
  GtkWidget *AlarmACK;
  char buf[32];
  struct tm tm;
  time_t viewtime;

  time (&viewtime);
  localtime_r (&viewtime, &tm);
  
  AlarmWin = gtk_dialog_new ();
  gtk_widget_set_name (AlarmWin, "AlarmWin");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "AlarmWin", AlarmWin);
  gtk_window_set_title (GTK_WINDOW (AlarmWin), ("Alarm!"));
  
#if GTK_MAJOR_VERSION >= 2
  GTK_WINDOW (AlarmWin)->type = GTK_WINDOW_TOPLEVEL;
#else
  GTK_WINDOW (AlarmWin)->type = GTK_WINDOW_DIALOG;
#endif
  
  gtk_window_set_position (GTK_WINDOW (AlarmWin), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (AlarmWin), TRUE);

  dialog_vbox3 = GTK_DIALOG (AlarmWin)->vbox;
  gtk_widget_set_name (dialog_vbox3, "dialog_vbox3");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "dialog_vbox3", dialog_vbox3);
  gtk_widget_show (dialog_vbox3);

  frame4 = gtk_frame_new (NULL);
  gtk_widget_set_name (frame4, "frame4");
  gtk_widget_ref (frame4);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "frame4", frame4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (frame4);
  gtk_box_pack_start (GTK_BOX (dialog_vbox3), frame4, TRUE, TRUE, 3);

  table6 = gtk_table_new (5, 2, FALSE);
  gtk_widget_set_name (table6, "table6");
  gtk_widget_ref (table6);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "table6", table6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table6);
  gtk_container_add (GTK_CONTAINER (frame4), table6);

  p = gpe_find_icon ("bell");
  pw = gpe_render_icon (AlarmWin->style, p);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "bell", pw,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pw);
  gtk_table_attach (GTK_TABLE (table6), pw, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 6);

  label38 = gtk_label_new (("Comment"));
  gtk_widget_set_name (label38, "label38");
  gtk_widget_ref (label38);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "label38", label38,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label38);
  gtk_table_attach (GTK_TABLE (table6), label38, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gtk_misc_set_alignment (GTK_MISC (label38), 0, 0.5);

  label37 = gtk_label_new (("Time"));
  gtk_widget_set_name (label37, "label37");
  gtk_widget_ref (label37);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "label37", label37,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label37);
  gtk_table_attach (GTK_TABLE (table6), label37, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gtk_misc_set_alignment (GTK_MISC (label37), 0, 0.5);

  label36 = gtk_label_new (("Date"));
  gtk_widget_set_name (label36, "label36");
  gtk_widget_ref (label36);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "label36", label36,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label36);
  gtk_table_attach (GTK_TABLE (table6), label36, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gtk_misc_set_alignment (GTK_MISC (label36), 0, 0.5);

  AlarmComment = gtk_entry_new ();
  gtk_widget_set_name (AlarmComment, "AlarmComment");
  gtk_widget_ref (AlarmComment);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmComment", AlarmComment,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmComment);
  gtk_table_attach (GTK_TABLE (table6), AlarmComment, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 2, 4);
  gtk_entry_set_text(GTK_ENTRY(AlarmComment),announcetext);
  gtk_entry_set_editable (GTK_ENTRY (AlarmComment), FALSE);

  strftime (buf, sizeof(buf), TIMEFMT, &tm);
  AlarmTime = gtk_label_new (buf);
  gtk_widget_set_name (AlarmTime, "AlarmTime");
  gtk_widget_ref (AlarmTime);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmTime", AlarmTime,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmTime);
  gtk_table_attach (GTK_TABLE (table6), AlarmTime, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_label_set_justify (GTK_LABEL (AlarmTime), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (AlarmTime), 0, 0.5);

  strftime (buf, sizeof(buf), DATEFMT, &tm);
  AlarmDate = gtk_label_new (buf);
  gtk_widget_set_name (AlarmDate, "AlarmDate");
  gtk_widget_ref (AlarmDate);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmDate", AlarmDate,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmDate);
  gtk_table_attach (GTK_TABLE (table6), AlarmDate, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_label_set_justify (GTK_LABEL (AlarmDate), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (AlarmDate), 0, 0.5);

  label32 = gtk_label_new (("Alarm time has been reached for:"));
  gtk_widget_set_name (label32, "label32");
  gtk_widget_ref (label32);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "label32", label32,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label32);
  gtk_table_attach (GTK_TABLE (table6), label32, 0, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 2, 5);
  gtk_label_set_justify (GTK_LABEL (label32), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label32), 0, 0.5);

  dialog_action_area3 = GTK_DIALOG (AlarmWin)->action_area;
  gtk_widget_set_name (dialog_action_area3, "dialog_action_area3");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "dialog_action_area3", dialog_action_area3);
  gtk_widget_show (dialog_action_area3);
  gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area3), 10);

  vbox6 = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (vbox6, "vbox6");
  gtk_widget_ref (vbox6);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "vbox6", vbox6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox6);
  gtk_box_pack_start (GTK_BOX (dialog_action_area3), vbox6, TRUE, TRUE, 0);

  hbuttonbox8 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox8, "hbuttonbox8");
  gtk_widget_ref (hbuttonbox8);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "hbuttonbox8", hbuttonbox8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox8);
  gtk_box_pack_start (GTK_BOX (vbox6), hbuttonbox8, TRUE, TRUE, 0);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbuttonbox8), 20);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (hbuttonbox8), 65, 25);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (hbuttonbox8), 3, 0);

  AlarmMute = gtk_button_new_with_label (("Mute"));
  gtk_widget_set_name (AlarmMute, "AlarmMute");
  gtk_widget_ref (AlarmMute);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmMute", AlarmMute,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmMute);
  gtk_container_add (GTK_CONTAINER (hbuttonbox8), AlarmMute);
  GTK_WIDGET_SET_FLAGS (AlarmMute, GTK_CAN_DEFAULT);

  AlarmDelay = gtk_button_new_with_label (("Snooze"));
  gtk_widget_set_name (AlarmDelay, "AlarmDelay");
  gtk_widget_ref (AlarmDelay);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmDelay", AlarmDelay,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmDelay);
  gtk_container_add (GTK_CONTAINER (hbuttonbox8), AlarmDelay);
  GTK_WIDGET_SET_FLAGS (AlarmDelay, GTK_CAN_DEFAULT);

  hbuttonbox7 = gtk_hbutton_box_new ();
  gtk_widget_set_name (hbuttonbox7, "hbuttonbox7");
  gtk_widget_ref (hbuttonbox7);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "hbuttonbox7", hbuttonbox7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox7);
  gtk_box_pack_start (GTK_BOX (vbox6), hbuttonbox7, TRUE, TRUE, 0);

  AlarmACK = gtk_button_new_with_label (("Acknowledge"));
  gtk_widget_set_name (AlarmACK, "AlarmACK");
  gtk_widget_ref (AlarmACK);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmACK", AlarmACK,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AlarmACK);
  gtk_container_add (GTK_CONTAINER (hbuttonbox7), AlarmACK);
  gtk_container_set_border_width (GTK_CONTAINER (AlarmACK), 4);
  GTK_WIDGET_SET_FLAGS (AlarmACK, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (AlarmMute), "clicked",
                      GTK_SIGNAL_FUNC (on_mute_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (AlarmDelay), "clicked",
                      GTK_SIGNAL_FUNC (on_snooze_clicked),
                      announcetext);
  gtk_signal_connect (GTK_OBJECT (AlarmACK), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_clicked),
                      NULL);
  bells_and_whistles();
  
  gtk_widget_grab_default (AlarmACK);
  return AlarmWin;
}

