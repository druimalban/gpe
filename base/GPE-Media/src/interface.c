/*
 * DO NOT EDIT THIS FILE - it is generated by Glade.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

GtkWidget*
create_GPE_Media (void)
{
  GtkWidget *GPE_Media;
  GtkWidget *vbox1;
  GtkWidget *menubar1;
  GtkWidget *file1;
  GtkWidget *file1_menu;
  GtkAccelGroup *file1_menu_accels;
  GtkWidget *exit1;
  GtkWidget *options1;
  GtkWidget *options1_menu;
  GtkAccelGroup *options1_menu_accels;
  GSList *RepType_group = NULL;
  GtkWidget *normal1;
  GtkWidget *repeat_1;
  GtkWidget *repeat_all1;
  GtkWidget *shuffle1;
  GtkWidget *help1;
  GtkWidget *help1_menu;
  GtkAccelGroup *help1_menu_accels;
  GtkWidget *about1;
  GtkWidget *Time;
  GtkWidget *Slider;
  GtkWidget *hbox1;
  GtkWidget *Start;
  GtkWidget *Stop;
  GtkWidget *Rew;
  GtkWidget *Pause;
  GtkWidget *Forw;
  GtkWidget *scrolledwindow1;
  GtkWidget *FileList;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label3;
  GtkWidget *hbox2;
  GtkWidget *AddList;
  GtkWidget *DeleteList;
  GtkWidget *vseparator1;
  GtkWidget *vbox2;
  GtkWidget *VolScale;
  GtkWidget *hbox3;
  GtkWidget *BassScale;
  GtkWidget *TrebleScale;

  GPE_Media = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (GPE_Media), "GPE_Media", GPE_Media);
  gtk_window_set_title (GTK_WINDOW (GPE_Media), _("GPE Media"));
  gtk_window_set_default_size (GTK_WINDOW (GPE_Media), 240, 300);
  gtk_window_set_policy (GTK_WINDOW (GPE_Media), TRUE, TRUE, FALSE);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (GPE_Media), vbox1);

  menubar1 = gtk_menu_bar_new ();
  gtk_widget_ref (menubar1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "menubar1", menubar1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (menubar1);
  gtk_box_pack_start (GTK_BOX (vbox1), menubar1, FALSE, FALSE, 0);

  file1 = gtk_menu_item_new_with_label (_("File"));
  gtk_widget_ref (file1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "file1", file1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (file1);
  gtk_container_add (GTK_CONTAINER (menubar1), file1);

  file1_menu = gtk_menu_new ();
  gtk_widget_ref (file1_menu);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "file1_menu", file1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (file1), file1_menu);
  file1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (file1_menu));

  exit1 = gtk_menu_item_new_with_label (_("Exit"));
  gtk_widget_ref (exit1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "exit1", exit1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (exit1);
  gtk_container_add (GTK_CONTAINER (file1_menu), exit1);

  options1 = gtk_menu_item_new_with_label (_("Options"));
  gtk_widget_ref (options1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "options1", options1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (options1);
  gtk_container_add (GTK_CONTAINER (menubar1), options1);

  options1_menu = gtk_menu_new ();
  gtk_widget_ref (options1_menu);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "options1_menu", options1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (options1), options1_menu);
  options1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (options1_menu));

  normal1 = gtk_radio_menu_item_new_with_label (RepType_group, _("Normal"));
  RepType_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (normal1));
  gtk_widget_ref (normal1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "normal1", normal1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (normal1);
  gtk_container_add (GTK_CONTAINER (options1_menu), normal1);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (normal1), TRUE);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (normal1), TRUE);

  repeat_1 = gtk_radio_menu_item_new_with_label (RepType_group, _("Repeat 1"));
  RepType_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (repeat_1));
  gtk_widget_ref (repeat_1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "repeat_1", repeat_1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (repeat_1);
  gtk_container_add (GTK_CONTAINER (options1_menu), repeat_1);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (repeat_1), TRUE);

  repeat_all1 = gtk_radio_menu_item_new_with_label (RepType_group, _("Repeat all"));
  RepType_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (repeat_all1));
  gtk_widget_ref (repeat_all1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "repeat_all1", repeat_all1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (repeat_all1);
  gtk_container_add (GTK_CONTAINER (options1_menu), repeat_all1);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (repeat_all1), TRUE);

  shuffle1 = gtk_radio_menu_item_new_with_label (RepType_group, _("Shuffle"));
  RepType_group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (shuffle1));
  gtk_widget_ref (shuffle1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "shuffle1", shuffle1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (shuffle1);
  gtk_container_add (GTK_CONTAINER (options1_menu), shuffle1);
  gtk_check_menu_item_set_show_toggle (GTK_CHECK_MENU_ITEM (shuffle1), TRUE);

  help1 = gtk_menu_item_new_with_label (_("Help"));
  gtk_widget_ref (help1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "help1", help1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (help1);
  gtk_container_add (GTK_CONTAINER (menubar1), help1);
  gtk_menu_item_right_justify (GTK_MENU_ITEM (help1));

  help1_menu = gtk_menu_new ();
  gtk_widget_ref (help1_menu);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "help1_menu", help1_menu,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (help1), help1_menu);
  help1_menu_accels = gtk_menu_ensure_uline_accel_group (GTK_MENU (help1_menu));

  about1 = gtk_menu_item_new_with_label (_("About"));
  gtk_widget_ref (about1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "about1", about1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (about1);
  gtk_container_add (GTK_CONTAINER (help1_menu), about1);

  Time = gtk_label_new (_("00:00:00"));
  gtk_widget_ref (Time);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Time", Time,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Time);
  gtk_box_pack_start (GTK_BOX (vbox1), Time, FALSE, FALSE, 2);

  Slider = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 0.1, 1, 0)));
  gtk_widget_ref (Slider);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Slider", Slider,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Slider);
  gtk_box_pack_start (GTK_BOX (vbox1), Slider, FALSE, FALSE, 2);
  gtk_scale_set_draw_value (GTK_SCALE (Slider), FALSE);
  gtk_range_set_update_policy (GTK_RANGE (Slider), GTK_UPDATE_DISCONTINUOUS);

  hbox1 = gtk_hbox_new (TRUE, 3);
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox1), 4);

  Start = gtk_button_new_with_label (_("Start"));
  gtk_widget_ref (Start);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Start", Start,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Start);
  gtk_box_pack_start (GTK_BOX (hbox1), Start, TRUE, TRUE, 0);

  Stop = gtk_button_new_with_label (_("Stop"));
  gtk_widget_ref (Stop);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Stop", Stop,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Stop);
  gtk_box_pack_start (GTK_BOX (hbox1), Stop, TRUE, TRUE, 0);

  Rew = gtk_button_new_with_label (_("Rew"));
  gtk_widget_ref (Rew);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Rew", Rew,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Rew);
  gtk_box_pack_start (GTK_BOX (hbox1), Rew, TRUE, TRUE, 0);

  Pause = gtk_button_new_with_label (_("Pause"));
  gtk_widget_ref (Pause);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Pause", Pause,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Pause);
  gtk_box_pack_start (GTK_BOX (hbox1), Pause, TRUE, TRUE, 0);

  Forw = gtk_button_new_with_label (_("Forw"));
  gtk_widget_ref (Forw);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "Forw", Forw,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (Forw);
  gtk_box_pack_start (GTK_BOX (hbox1), Forw, TRUE, TRUE, 0);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 3);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  FileList = gtk_clist_new (3);
  gtk_widget_ref (FileList);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "FileList", FileList,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (FileList);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), FileList);
  gtk_clist_set_column_width (GTK_CLIST (FileList), 0, 16);
  gtk_clist_set_column_width (GTK_CLIST (FileList), 1, 148);
  gtk_clist_set_column_width (GTK_CLIST (FileList), 2, 40);
  gtk_clist_column_titles_show (GTK_CLIST (FileList));

  label1 = gtk_label_new (_("Nr"));
  gtk_widget_ref (label1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "label1", label1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label1);
  gtk_clist_set_column_widget (GTK_CLIST (FileList), 0, label1);

  label2 = gtk_label_new (_("Name"));
  gtk_widget_ref (label2);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "label2", label2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label2);
  gtk_clist_set_column_widget (GTK_CLIST (FileList), 1, label2);

  label3 = gtk_label_new (_("Time"));
  gtk_widget_ref (label3);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "label3", label3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label3);
  gtk_clist_set_column_widget (GTK_CLIST (FileList), 2, label3);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_widget_ref (hbox2);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "hbox2", hbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox2, FALSE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox2), 4);

  AddList = gtk_button_new_with_label (_(" Add "));
  gtk_widget_ref (AddList);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "AddList", AddList,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (AddList);
  gtk_box_pack_start (GTK_BOX (hbox2), AddList, FALSE, FALSE, 0);

  DeleteList = gtk_button_new_with_label (_(" Delete "));
  gtk_widget_ref (DeleteList);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "DeleteList", DeleteList,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (DeleteList);
  gtk_box_pack_start (GTK_BOX (hbox2), DeleteList, FALSE, FALSE, 0);

  vseparator1 = gtk_vseparator_new ();
  gtk_widget_ref (vseparator1);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "vseparator1", vseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vseparator1);
  gtk_box_pack_start (GTK_BOX (hbox2), vseparator1, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox2);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "vbox2", vbox2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox2);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox2, TRUE, TRUE, 0);

  VolScale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 0.1, 1, 0)));
  gtk_widget_ref (VolScale);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "VolScale", VolScale,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (VolScale);
  gtk_box_pack_start (GTK_BOX (vbox2), VolScale, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (VolScale), FALSE);

  hbox3 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox3);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "hbox3", hbox3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox3);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox3, TRUE, TRUE, 0);

  BassScale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 0.1, 1, 0)));
  gtk_widget_ref (BassScale);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "BassScale", BassScale,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (BassScale);
  gtk_box_pack_start (GTK_BOX (hbox3), BassScale, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (BassScale), FALSE);

  TrebleScale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 0.1, 1, 0)));
  gtk_widget_ref (TrebleScale);
  gtk_object_set_data_full (GTK_OBJECT (GPE_Media), "TrebleScale", TrebleScale,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (TrebleScale);
  gtk_box_pack_start (GTK_BOX (hbox3), TrebleScale, TRUE, TRUE, 0);
  gtk_scale_set_draw_value (GTK_SCALE (TrebleScale), FALSE);

  gtk_signal_connect (GTK_OBJECT (GPE_Media), "destroy",
                      GTK_SIGNAL_FUNC (on_exit1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (GPE_Media), "delete_event",
                      GTK_SIGNAL_FUNC (on_exit1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (GPE_Media), "destroy_event",
                      GTK_SIGNAL_FUNC (on_exit1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (exit1), "activate",
                      GTK_SIGNAL_FUNC (on_exit1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (normal1), "activate",
                      GTK_SIGNAL_FUNC (on_normal1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (repeat_1), "activate",
                      GTK_SIGNAL_FUNC (on_repeat_1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (repeat_all1), "activate",
                      GTK_SIGNAL_FUNC (on_repeat_all1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (shuffle1), "activate",
                      GTK_SIGNAL_FUNC (on_shuffle1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (about1), "activate",
                      GTK_SIGNAL_FUNC (on_about1_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Start), "clicked",
                      GTK_SIGNAL_FUNC (on_Start_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Stop), "clicked",
                      GTK_SIGNAL_FUNC (on_Stop_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Rew), "clicked",
                      GTK_SIGNAL_FUNC (on_Rew_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Pause), "clicked",
                      GTK_SIGNAL_FUNC (on_Pause_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (Forw), "clicked",
                      GTK_SIGNAL_FUNC (on_Forw_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (FileList), "select_row",
                      GTK_SIGNAL_FUNC (on_FileList_select_row),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (AddList), "clicked",
                      GTK_SIGNAL_FUNC (on_AddList_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (DeleteList), "clicked",
                      GTK_SIGNAL_FUNC (on_DeleteList_clicked),
                      NULL);

  return GPE_Media;
}

GtkWidget*
create_fileselection (void)
{
  GtkWidget *fileselection;
  GtkWidget *ok_button1;
  GtkWidget *cancel_button1;

  fileselection = gtk_file_selection_new (_("Select File"));
  gtk_object_set_data (GTK_OBJECT (fileselection), "fileselection", fileselection);
  gtk_window_set_default_size (GTK_WINDOW (fileselection), 240, 300);
  gtk_window_set_policy (GTK_WINDOW (fileselection), TRUE, TRUE, TRUE);
  gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (fileselection));

  ok_button1 = GTK_FILE_SELECTION (fileselection)->ok_button;
  gtk_object_set_data (GTK_OBJECT (fileselection), "ok_button1", ok_button1);
  gtk_widget_show (ok_button1);
  GTK_WIDGET_SET_FLAGS (ok_button1, GTK_CAN_DEFAULT);

  cancel_button1 = GTK_FILE_SELECTION (fileselection)->cancel_button;
  gtk_object_set_data (GTK_OBJECT (fileselection), "cancel_button1", cancel_button1);
  gtk_widget_show (cancel_button1);
  GTK_WIDGET_SET_FLAGS (cancel_button1, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (ok_button1), "clicked",
                      GTK_SIGNAL_FUNC (on_filesel_ok_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (cancel_button1), "clicked",
                      GTK_SIGNAL_FUNC (on_filesel_cancel_clicked),
                      NULL);

  return fileselection;
}

