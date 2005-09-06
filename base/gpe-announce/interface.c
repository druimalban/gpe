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
#include <libintl.h>

#include <gtk/gtk.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#include "announce.h"

#define _(x) gettext(x)

GtkWidget*
create_window (char *announcetext)
{
  GtkWidget *AlarmWin;
  GtkWidget *DialogVBox;
  GtkWidget *InfoFrame;
  GtkWidget *InfoTable;
  GdkPixbuf *BellPixbuf;
  GtkWidget *BellImage;
  GtkWidget *CommentLabel;
  GtkWidget *TimeLabel;
  GtkWidget *DateLabel;
  GtkWidget *AlarmComment;
  GtkWidget *AlarmTime;
  GtkWidget *AlarmDate;
  GtkWidget *HeadingLabel;
  GtkWidget *SnoozeHBox;
  GtkWidget *SnoozeLabel;
  GtkObject *HoursAdj;
  GtkWidget *HoursSpin;
  GtkWidget *HoursLabel;
  GtkObject *MinutesAdj;
  GtkWidget *MinutesSpin;
  GtkWidget *MinutesLabel;
  GtkWidget *DialogActionArea;
  GtkWidget *ActionVbox;
  GtkWidget *TopButtonBox;
  GtkWidget *AlarmMute;
  GtkWidget *AlarmSnooze;
  GtkWidget *BtmButtonBox;
  GtkWidget *AlarmACK;
  char buf[32];
  struct tm tm;
  time_t viewtime;

  time (&viewtime);
  localtime_r (&viewtime, &tm);
  
  AlarmWin = gtk_dialog_new ();
  gtk_widget_set_name (AlarmWin, "AlarmWin");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "AlarmWin", AlarmWin);
  gtk_window_set_title (GTK_WINDOW (AlarmWin), _("Alarm!"));
  
  gtk_window_set_position (GTK_WINDOW (AlarmWin), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (AlarmWin), TRUE);

  DialogVBox = GTK_DIALOG (AlarmWin)->vbox;
  gtk_widget_set_name (DialogVBox, "DialogVBox");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "DialogVBox", DialogVBox);

  InfoFrame = gtk_frame_new (NULL);
  gtk_widget_set_name (InfoFrame, "InfoFrame");
  gtk_widget_ref (InfoFrame);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "InfoFrame", InfoFrame,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (DialogVBox), InfoFrame, TRUE, TRUE, 3);

  InfoTable = gtk_table_new (5, 2, FALSE);
  gtk_widget_set_name (InfoTable, "InfoTable");
  gtk_widget_ref (InfoTable);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "InfoTable", InfoTable,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (InfoFrame), InfoTable);

  BellPixbuf = gpe_find_icon ("bell");
  BellImage = gtk_image_new_from_pixbuf(BellPixbuf);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "bell", BellImage,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (InfoTable), BellImage, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 6);

  if (announcetext)
    {
      CommentLabel = gtk_label_new (_("Comment"));
      gtk_widget_set_name (CommentLabel, "CommentLabel");
      gtk_widget_ref (CommentLabel);
      gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "CommentLabel",
				CommentLabel,
				(GtkDestroyNotify) gtk_widget_unref);
      gtk_table_attach (GTK_TABLE (InfoTable), CommentLabel, 0, 1, 4, 5,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (0), 2, 0);
      gtk_misc_set_alignment (GTK_MISC (CommentLabel), 0, 0.5);
    }

  TimeLabel = gtk_label_new (_("Time"));
  gtk_widget_set_name (TimeLabel, "TimeLabel");
  gtk_widget_ref (TimeLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "TimeLabel", TimeLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (TimeLabel);
  gtk_table_attach (GTK_TABLE (InfoTable), TimeLabel, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gtk_misc_set_alignment (GTK_MISC (TimeLabel), 0, 0.5);

  DateLabel = gtk_label_new (_("Date"));
  gtk_widget_set_name (DateLabel, "DateLabel");
  gtk_widget_ref (DateLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "DateLabel", DateLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (InfoTable), DateLabel, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 0);
  gtk_misc_set_alignment (GTK_MISC (DateLabel), 0, 0.5);

  if (announcetext)
    {
      AlarmComment = gtk_entry_new ();
      gtk_widget_set_name (AlarmComment, "AlarmComment");
      gtk_widget_ref (AlarmComment);
      gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmComment",
				AlarmComment,
				(GtkDestroyNotify) gtk_widget_unref);
      gtk_table_attach (GTK_TABLE (InfoTable), AlarmComment, 1, 2, 4, 5,
			(GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			(GtkAttachOptions) (0), 2, 4);

      gtk_entry_set_text(GTK_ENTRY(AlarmComment),announcetext);
      gtk_entry_set_editable (GTK_ENTRY (AlarmComment), FALSE);
      GTK_WIDGET_UNSET_FLAGS (AlarmComment, GTK_CAN_FOCUS);
    }

  strftime (buf, sizeof(buf), TIMEFMT, &tm);
  AlarmTime = gtk_label_new (buf);
  gtk_widget_set_name (AlarmTime, "AlarmTime");
  gtk_widget_ref (AlarmTime);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmTime", AlarmTime,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (InfoTable), AlarmTime, 1, 2, 3, 4,
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
  gtk_table_attach (GTK_TABLE (InfoTable), AlarmDate, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 2, 2);
  gtk_label_set_justify (GTK_LABEL (AlarmDate), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (AlarmDate), 0, 0.5);

  HeadingLabel = gtk_label_new (_("Alarm time has been reached for:"));
  gtk_widget_set_name (HeadingLabel, "HeadingLabel");
  gtk_widget_ref (HeadingLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "HeadingLabel", HeadingLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (InfoTable), HeadingLabel, 0, 2, 1, 2,
                    (GtkAttachOptions) (0),
                    (GtkAttachOptions) (0), 2, 5);
  gtk_label_set_justify (GTK_LABEL (HeadingLabel), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (HeadingLabel), 0, 0.5);

  SnoozeHBox = gtk_hbox_new (FALSE, 1);
  gtk_widget_set_name (SnoozeHBox, "SnoozeHBox");
  gtk_widget_ref (SnoozeHBox);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "SnoozeHBox", SnoozeHBox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (DialogVBox), SnoozeHBox, FALSE, TRUE, 3);

  SnoozeLabel = gtk_label_new (_("Snooze for"));
  gtk_widget_set_name (SnoozeLabel, "SnoozeLabel");
  gtk_widget_ref (SnoozeLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "SnoozeLabel",
			    SnoozeLabel,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (SnoozeHBox), SnoozeLabel, FALSE, FALSE, 1);

  HoursAdj = gtk_adjustment_new (0.0, 0.0, 23.0, 1.0, 6.0, 6.0);
  HoursSpin = gtk_spin_button_new (GTK_ADJUSTMENT (HoursAdj), 1.0, 0);
  gtk_widget_set_name (HoursSpin, "HoursSpin");
  gtk_widget_ref (HoursSpin);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "HoursSpin",
			    HoursSpin,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (SnoozeHBox), HoursSpin, FALSE, FALSE, 1);

  HoursLabel = gtk_label_new (_("hrs"));
  gtk_widget_set_name (HoursLabel, "HoursLabel");
  gtk_widget_ref (HoursLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "HoursLabel",
			    HoursLabel,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (SnoozeHBox), HoursLabel, FALSE, FALSE, 1);

  MinutesAdj = gtk_adjustment_new (5.0, 0.0, 59.0, 1.0, 5.0, 5.0);
  MinutesSpin = gtk_spin_button_new (GTK_ADJUSTMENT (MinutesAdj), 1.0, 0);
  gtk_widget_set_name (MinutesSpin, "MinutesSpin");
  gtk_widget_ref (MinutesSpin);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "MinutesSpin",
			    MinutesSpin,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (SnoozeHBox), MinutesSpin, FALSE, FALSE, 1);

  MinutesLabel = gtk_label_new (_("mins"));
  gtk_widget_set_name (MinutesLabel, "MinutesLabel");
  gtk_widget_ref (MinutesLabel);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "MinutesLabel",
			    MinutesLabel,
			    (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (SnoozeHBox), MinutesLabel, FALSE, FALSE, 1);

  DialogActionArea = GTK_DIALOG (AlarmWin)->action_area;
  gtk_widget_set_name (DialogActionArea, "DialogActionArea");
  gtk_object_set_data (GTK_OBJECT (AlarmWin), "DialogActionArea",
		       DialogActionArea);
  gtk_widget_show (DialogActionArea);
  gtk_container_set_border_width (GTK_CONTAINER (DialogActionArea), 10);

  ActionVbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_set_name (ActionVbox, "ActionVbox");
  gtk_widget_ref (ActionVbox);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "ActionVbox", ActionVbox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (DialogActionArea), ActionVbox, TRUE, TRUE, 0);

  TopButtonBox = gtk_hbutton_box_new ();
  gtk_widget_set_name (TopButtonBox, "TopButtonBox");
  gtk_widget_ref (TopButtonBox);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "TopButtonBox", TopButtonBox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (ActionVbox), TopButtonBox, TRUE, TRUE, 0);
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (TopButtonBox), 20);
  gtk_button_box_set_child_size (GTK_BUTTON_BOX (TopButtonBox), 65, 25);
  gtk_button_box_set_child_ipadding (GTK_BUTTON_BOX (TopButtonBox), 3, 0);

  AlarmMute = gtk_button_new_with_label (_("Mute"));
  gtk_widget_set_name (AlarmMute, "AlarmMute");
  gtk_widget_ref (AlarmMute);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmMute", AlarmMute,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (TopButtonBox), AlarmMute);
  GTK_WIDGET_SET_FLAGS (AlarmMute, GTK_CAN_DEFAULT);

  AlarmSnooze = gpe_picture_button (NULL, _("Snooze"), "clock-popup");
  gtk_widget_set_name (AlarmSnooze, "AlarmSnooze");
  gtk_widget_ref (AlarmSnooze);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmSnooze", AlarmSnooze,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (TopButtonBox), AlarmSnooze);
  GTK_WIDGET_SET_FLAGS (AlarmSnooze, GTK_CAN_DEFAULT);

  BtmButtonBox = gtk_hbutton_box_new ();
  gtk_widget_set_name (BtmButtonBox, "BtmButtonBox");
  gtk_widget_ref (BtmButtonBox);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "BtmButtonBox", BtmButtonBox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (ActionVbox), BtmButtonBox, TRUE, TRUE, 0);

  AlarmACK = gpe_picture_button (NULL, _("Dismiss"), "!gtk-close");
  gtk_widget_set_name (AlarmACK, "AlarmACK");
  gtk_widget_ref (AlarmACK);
  gtk_object_set_data_full (GTK_OBJECT (AlarmWin), "AlarmACK", AlarmACK,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (BtmButtonBox), AlarmACK);
  gtk_container_set_border_width (GTK_CONTAINER (AlarmACK), 4);
  GTK_WIDGET_SET_FLAGS (AlarmACK, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (AlarmSnooze), "clicked",
                      GTK_SIGNAL_FUNC (on_snooze_clicked),
                      AlarmWin);
  gtk_signal_connect (GTK_OBJECT (AlarmMute), "clicked",
                      GTK_SIGNAL_FUNC (on_mute_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (AlarmACK), "clicked",
                      GTK_SIGNAL_FUNC (on_ok_clicked),
                      NULL);
  bells_and_whistles();
  
  gtk_widget_grab_default (AlarmACK);
  return AlarmWin;
}

