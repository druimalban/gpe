
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#define SPACE	3
#define	BUT_X	40
#define	BUT_Y	22

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  gtk_object_set_data_full (GTK_OBJECT (component), name, \
    gtk_widget_ref (widget), (GtkDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  gtk_object_set_data (GTK_OBJECT (component), name, widget)

GtkWidget*
create_main_window (void)
{
  gint i, j;
  GtkWidget *main_window;
  GtkWidget *window_vbox;
  GtkWidget *table_all;
  GtkWidget *scrolledwindow1;
  GtkWidget *textview;
  GtkWidget *button_backspace;
  GtkWidget *button_clr;
  GtkWidget *button_allclr;
  GtkWidget *button_econst;
  GtkWidget *button_piconst;
  GtkWidget *button_ee;
  GtkWidget *tbutton_inv;
  GtkWidget *tbutton_hyp;
  GtkWidget *button_fac;
  GtkWidget *button_sin;
  GtkWidget *button_cos;
  GtkWidget *button_tan;
  GtkWidget *button_reci;
  GtkWidget *button_log;
  GtkWidget *button_ln;
  GtkWidget *button_sq;
  GtkWidget *button_pow;
  GtkWidget *button_sqrt;
  GtkWidget *button_paropen;
  GtkWidget *button_parclose;
  GtkWidget *button_7;
  GtkWidget *button_8;
  GtkWidget *button_9;
  GtkWidget *button_div;
  GtkWidget *button_4;
  GtkWidget *button_1;
  GtkWidget *button_0;
  GtkWidget *button_5;
  GtkWidget *button_2;
  GtkWidget *button_point;
  GtkWidget *button_6;
  GtkWidget *button_3;
  GtkWidget *button_minus;
  GtkWidget *button_plus;
  GtkWidget *button_enter;
  GtkWidget *button_sign;
  GtkWidget *button_MS;
  GtkWidget *button_MR;
  GtkWidget *button_Mplus;
  GtkWidget *button_mult;
  GtkWidget *statusbar;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (main_window), _("layout galculator"));

  window_vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (window_vbox);
  gtk_container_add (GTK_CONTAINER (main_window), window_vbox);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (window_vbox), scrolledwindow1, FALSE, FALSE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

  textview = gtk_text_view_new ();
  gtk_widget_show (textview);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), textview);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (textview), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (textview), 10);
  gtk_text_view_set_right_margin (GTK_TEXT_VIEW (textview), 10);

  table_all = gtk_table_new (8, 5, FALSE);
  gtk_widget_show (table_all);
  gtk_box_pack_start (GTK_BOX (window_vbox), table_all, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (table_all), SPACE);
  gtk_table_set_row_spacings (GTK_TABLE (table_all), SPACE);
  gtk_table_set_col_spacings (GTK_TABLE (table_all), SPACE);

  i=0;
  j=0;
  
  button_reci = gtk_button_new_with_mnemonic (_("1/x"));
  gtk_widget_show (button_reci);
  gtk_table_attach (GTK_TABLE (table_all), button_reci, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_reci, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/
  
  button_sq = gtk_button_new_with_mnemonic (_("x^2"));
  gtk_widget_show (button_sq);
  gtk_table_attach (GTK_TABLE (table_all), button_sq, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_sq, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_sqrt = gtk_button_new_with_mnemonic (_("SQRT"));
  gtk_widget_show (button_sqrt);
  gtk_table_attach (GTK_TABLE (table_all), button_sqrt, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_sqrt, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_clr = gtk_button_new_with_mnemonic (_("C"));
  gtk_widget_show (button_clr);
  gtk_table_attach (GTK_TABLE (table_all), button_clr, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_clr, BUT_X, BUT_Y);
  
  gtk_widget_add_accelerator (button_clr, "clicked", accel_group,
                              GDK_Delete, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_allclr = gtk_button_new_with_mnemonic (_("AC"));
  gtk_widget_show (button_allclr);
  gtk_table_attach (GTK_TABLE (table_all), button_allclr, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_allclr, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_allclr, "clicked", accel_group,
                              GDK_Delete, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i=0;
  j++;
 /***************/

  tbutton_inv = gtk_toggle_button_new_with_mnemonic (_("INV"));
  gtk_widget_show (tbutton_inv);
  gtk_table_attach (GTK_TABLE (table_all), tbutton_inv, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (tbutton_inv, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  tbutton_hyp = gtk_toggle_button_new_with_mnemonic (_("HYP"));
  gtk_widget_show (tbutton_hyp);
  gtk_table_attach (GTK_TABLE (table_all), tbutton_hyp, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (tbutton_hyp, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_sin = gtk_button_new_with_mnemonic (_("sin"));
  gtk_widget_show (button_sin);
  gtk_table_attach (GTK_TABLE (table_all), button_sin, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_sin, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_cos = gtk_button_new_with_mnemonic (_("cos"));
  gtk_widget_show (button_cos);
  gtk_table_attach (GTK_TABLE (table_all), button_cos, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_cos, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_tan = gtk_button_new_with_mnemonic (_("tan"));
  gtk_widget_show (button_tan);
  gtk_table_attach (GTK_TABLE (table_all), button_tan, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_tan, BUT_X, BUT_Y);

 /***************/
  i=0;
  j++;
 /***************/

  button_econst = gtk_button_new_with_mnemonic (_("e"));
  gtk_widget_show (button_econst);
  gtk_table_attach (GTK_TABLE (table_all), button_econst, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_econst, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_ee = gtk_button_new_with_mnemonic (_("EE"));
  gtk_widget_show (button_ee);
  gtk_table_attach (GTK_TABLE (table_all), button_ee, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_ee, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_log = gtk_button_new_with_mnemonic (_("log"));
  gtk_widget_show (button_log);
  gtk_table_attach (GTK_TABLE (table_all), button_log, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_log, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_ln = gtk_button_new_with_mnemonic (_("ln"));
  gtk_widget_show (button_ln);
  gtk_table_attach (GTK_TABLE (table_all), button_ln, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_ln, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_pow = gtk_button_new_with_mnemonic (_("x^y"));
  gtk_widget_show (button_pow);
  gtk_table_attach (GTK_TABLE (table_all), button_pow, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_pow, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_pow, "clicked", accel_group,
                              GDK_asciicircum, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i=0;
  j++;
 /***************/

  button_piconst = gtk_button_new_with_mnemonic (_("PI"));
  gtk_widget_show (button_piconst);
  gtk_table_attach (GTK_TABLE (table_all), button_piconst, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_piconst, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_fac = gtk_button_new_with_mnemonic (_("n!"));
  gtk_widget_show (button_fac);
  gtk_table_attach (GTK_TABLE (table_all), button_fac, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_fac, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_paropen = gtk_button_new_with_mnemonic (_("("));
  gtk_widget_show (button_paropen);
  gtk_table_attach (GTK_TABLE (table_all), button_paropen, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_paropen, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_paropen, "clicked", accel_group,
                              GDK_parenleft, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_parclose = gtk_button_new_with_mnemonic (_(")"));
  gtk_widget_show (button_parclose);
  gtk_table_attach (GTK_TABLE (table_all), button_parclose, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_parclose, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_parclose, "clicked", accel_group,
                              GDK_parenright, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_div = gtk_button_new_with_mnemonic (_("/"));
  gtk_widget_show (button_div);
  gtk_table_attach (GTK_TABLE (table_all), button_div, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_div, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_div, "clicked", accel_group,
                              GDK_KP_Divide, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_div, "clicked", accel_group,
                              GDK_slash, 0,
                              GTK_ACCEL_VISIBLE);
  
 /***************/
  i=0;
  j++;
 /***************/

  button_MS = gtk_button_new_with_mnemonic (_("MS"));
  gtk_widget_show (button_MS);
  gtk_table_attach (GTK_TABLE (table_all), button_MS, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_MS, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_7 = gtk_button_new_with_mnemonic (_("7"));
  gtk_widget_show (button_7);
  gtk_table_attach (GTK_TABLE (table_all), button_7, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_7, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_7, "clicked", accel_group,
                              GDK_7, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_7, "clicked", accel_group,
                              GDK_KP_7, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_8 = gtk_button_new_with_mnemonic (_("8"));
  gtk_widget_show (button_8);
  gtk_table_attach (GTK_TABLE (table_all), button_8, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_8, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_8, "clicked", accel_group,
                              GDK_8, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_8, "clicked", accel_group,
                              GDK_KP_8, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_9 = gtk_button_new_with_mnemonic (_("9"));
  gtk_widget_show (button_9);
  gtk_table_attach (GTK_TABLE (table_all), button_9, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_9, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_9, "clicked", accel_group,
                              GDK_9, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_9, "clicked", accel_group,
                              GDK_KP_9, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_mult = gtk_button_new_with_mnemonic (_("*"));
  gtk_widget_show (button_mult);
  gtk_table_attach (GTK_TABLE (table_all), button_mult, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_mult, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_mult, "clicked", accel_group,
                              GDK_KP_Multiply, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_mult, "clicked", accel_group,
                              GDK_asterisk, 0,
                              GTK_ACCEL_VISIBLE);
  
 /***************/
  i=0;
  j++;
 /***************/

  button_MR = gtk_button_new_with_mnemonic (_("MR"));
  gtk_widget_show (button_MR);
  gtk_table_attach (GTK_TABLE (table_all), button_MR, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_MR, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_4 = gtk_button_new_with_mnemonic (_("4"));
  gtk_widget_show (button_4);
  gtk_table_attach (GTK_TABLE (table_all), button_4, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_4, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_4, "clicked", accel_group,
                              GDK_4, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_4, "clicked", accel_group,
                              GDK_KP_4, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_5 = gtk_button_new_with_mnemonic (_("5"));
  gtk_widget_show (button_5);
  gtk_table_attach (GTK_TABLE (table_all), button_5, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_5, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_5, "clicked", accel_group,
                              GDK_5, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_5, "clicked", accel_group,
                              GDK_KP_5, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_6 = gtk_button_new_with_mnemonic (_("6"));
  gtk_widget_show (button_6);
  gtk_table_attach (GTK_TABLE (table_all), button_6, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_6, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_6, "clicked", accel_group,
                              GDK_6, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_6, "clicked", accel_group,
                              GDK_KP_6, 0,
                              GTK_ACCEL_VISIBLE);
  
 /***************/
  i++;
 /***************/

  button_minus = gtk_button_new_with_mnemonic (_("-"));
  gtk_widget_show (button_minus);
  gtk_table_attach (GTK_TABLE (table_all), button_minus, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_minus, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_minus, "clicked", accel_group,
                              GDK_KP_Subtract, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_minus, "clicked", accel_group,
                              GDK_minus, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i=0;
  j++;
 /***************/

  button_Mplus = gtk_button_new_with_mnemonic (_("M+"));
  gtk_widget_show (button_Mplus);
  gtk_table_attach (GTK_TABLE (table_all), button_Mplus, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_Mplus, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_1 = gtk_button_new_with_mnemonic (_("1"));
  gtk_widget_show (button_1);
  gtk_table_attach (GTK_TABLE (table_all), button_1, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_1, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_1, "clicked", accel_group,
                              GDK_1, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_1, "clicked", accel_group,
                              GDK_KP_1, 0,
                              GTK_ACCEL_VISIBLE);
 /***************/
  i++;
 /***************/

  button_2 = gtk_button_new_with_mnemonic (_("2"));
  gtk_widget_show (button_2);
  gtk_table_attach (GTK_TABLE (table_all), button_2, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_2, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_2, "clicked", accel_group,
                              GDK_2, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_2, "clicked", accel_group,
                              GDK_KP_2, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_3 = gtk_button_new_with_mnemonic (_("3"));
  gtk_widget_show (button_3);
  gtk_table_attach (GTK_TABLE (table_all), button_3, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_3, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_3, "clicked", accel_group,
                              GDK_3, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_3, "clicked", accel_group,
                              GDK_KP_3, 0,
                              GTK_ACCEL_VISIBLE);
  
 /***************/
  i++;
 /***************/

  button_plus = gtk_button_new_with_mnemonic (_("+"));
  gtk_widget_show (button_plus);
  gtk_table_attach (GTK_TABLE (table_all), button_plus, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_plus, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_plus, "clicked", accel_group,
                              GDK_KP_Add, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_plus, "clicked", accel_group,
                              GDK_plus, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i=0;
  j++;
 /***************/

  button_backspace = gtk_button_new_with_mnemonic (_("<-"));
  gtk_widget_show (button_backspace);
  gtk_table_attach (GTK_TABLE (table_all), button_backspace, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_backspace, BUT_X, 22);
  gtk_widget_add_accelerator (button_backspace, "clicked", accel_group,
                              GDK_BackSpace, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_0 = gtk_button_new_with_mnemonic (_("0"));
  gtk_widget_show (button_0);
  gtk_table_attach (GTK_TABLE (table_all), button_0, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_0, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_0, "clicked", accel_group,
                              GDK_0, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_0, "clicked", accel_group,
                              GDK_KP_0, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_point = gtk_button_new_with_mnemonic (_("."));
  gtk_widget_show (button_point);
  gtk_table_attach (GTK_TABLE (table_all), button_point, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_point, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_point, "clicked", accel_group,
                              GDK_comma, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_point, "clicked", accel_group,
                              GDK_period, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_point, "clicked", accel_group,
                              GDK_KP_Decimal, 0,
                              GTK_ACCEL_VISIBLE);

 /***************/
  i++;
 /***************/

  button_sign = gtk_button_new_with_mnemonic (_("+/-"));
  gtk_widget_show (button_sign);
  gtk_table_attach (GTK_TABLE (table_all), button_sign, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_sign, BUT_X, BUT_Y);

 /***************/
  i++;
 /***************/

  button_enter = gtk_button_new_with_mnemonic (_("="));
  gtk_widget_show (button_enter);
  gtk_table_attach (GTK_TABLE (table_all), button_enter, i, i+1, j, j+1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_widget_set_usize (button_enter, BUT_X, BUT_Y);
  gtk_widget_add_accelerator (button_enter, "clicked", accel_group,
                              GDK_Return, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_enter, "clicked", accel_group,
                              GDK_KP_Enter, 0,
                              GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (button_enter, "clicked", accel_group,
                              GDK_equal, 0,
                              GTK_ACCEL_VISIBLE);

  statusbar = gtk_statusbar_new ();
  gtk_widget_show (statusbar);
  gtk_box_pack_start (GTK_BOX (window_vbox), statusbar, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "key_press_event",
                      GTK_SIGNAL_FUNC (on_main_window_key_press_event),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_backspace), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_clr), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_allclr), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_econst), "clicked",
                      GTK_SIGNAL_FUNC (on_constant_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_piconst), "clicked",
                      GTK_SIGNAL_FUNC (on_constant_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_ee), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (tbutton_inv), "clicked",
                      GTK_SIGNAL_FUNC (on_tbutton_fmod_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (tbutton_hyp), "clicked",
                      GTK_SIGNAL_FUNC (on_tbutton_fmod_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_fac), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_sin), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_cos), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_tan), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_reci), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_log), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_ln), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_sq), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_pow), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_sqrt), "clicked",
                      GTK_SIGNAL_FUNC (on_function_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_paropen), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_parclose), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_7), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_8), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_9), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_div), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_4), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_1), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_0), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_5), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_2), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_point), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_6), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_3), "clicked",
                      GTK_SIGNAL_FUNC (on_number_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_minus), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_plus), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_enter), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_sign), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_MS), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_MR), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_Mplus), "clicked",
                      GTK_SIGNAL_FUNC (on_gfunc_button_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (button_mult), "clicked",
                      GTK_SIGNAL_FUNC (on_operation_button_clicked),
                      NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (main_window, main_window, "main_window");
  GLADE_HOOKUP_OBJECT (main_window, window_vbox, "window_vbox");
  GLADE_HOOKUP_OBJECT (main_window, scrolledwindow1, "scrolledwindow1");
  GLADE_HOOKUP_OBJECT (main_window, textview, "textview");
  GLADE_HOOKUP_OBJECT (main_window, button_backspace, "button_backspace");
  GLADE_HOOKUP_OBJECT (main_window, button_clr, "button_clr");
  GLADE_HOOKUP_OBJECT (main_window, button_allclr, "button_allclr");
  GLADE_HOOKUP_OBJECT (main_window, button_econst, "button_econst");
  GLADE_HOOKUP_OBJECT (main_window, button_piconst, "button_piconst");
  GLADE_HOOKUP_OBJECT (main_window, button_ee, "button_ee");
  GLADE_HOOKUP_OBJECT (main_window, tbutton_inv, "tbutton_inv");
  GLADE_HOOKUP_OBJECT (main_window, tbutton_hyp, "tbutton_hyp");
  GLADE_HOOKUP_OBJECT (main_window, button_fac, "button_fac");
  GLADE_HOOKUP_OBJECT (main_window, button_sin, "button_sin");
  GLADE_HOOKUP_OBJECT (main_window, button_cos, "button_cos");
  GLADE_HOOKUP_OBJECT (main_window, button_tan, "button_tan");
  GLADE_HOOKUP_OBJECT (main_window, button_reci, "button_reci");
  GLADE_HOOKUP_OBJECT (main_window, button_log, "button_log");
  GLADE_HOOKUP_OBJECT (main_window, button_ln, "button_ln");
  GLADE_HOOKUP_OBJECT (main_window, button_sq, "button_sq");
  GLADE_HOOKUP_OBJECT (main_window, button_pow, "button_pow");
  GLADE_HOOKUP_OBJECT (main_window, button_sqrt, "button_sqrt");
  GLADE_HOOKUP_OBJECT (main_window, button_paropen, "button_paropen");
  GLADE_HOOKUP_OBJECT (main_window, button_parclose, "button_parclose");
  GLADE_HOOKUP_OBJECT (main_window, button_7, "button_7");
  GLADE_HOOKUP_OBJECT (main_window, button_8, "button_8");
  GLADE_HOOKUP_OBJECT (main_window, button_9, "button_9");
  GLADE_HOOKUP_OBJECT (main_window, button_div, "button_div");
  GLADE_HOOKUP_OBJECT (main_window, button_4, "button_4");
  GLADE_HOOKUP_OBJECT (main_window, button_1, "button_1");
  GLADE_HOOKUP_OBJECT (main_window, button_0, "button_0");
  GLADE_HOOKUP_OBJECT (main_window, button_5, "button_5");
  GLADE_HOOKUP_OBJECT (main_window, button_2, "button_2");
  GLADE_HOOKUP_OBJECT (main_window, button_point, "button_point");
  GLADE_HOOKUP_OBJECT (main_window, button_6, "button_6");
  GLADE_HOOKUP_OBJECT (main_window, button_3, "button_3");
  GLADE_HOOKUP_OBJECT (main_window, button_minus, "button_minus");
  GLADE_HOOKUP_OBJECT (main_window, button_plus, "button_plus");
  GLADE_HOOKUP_OBJECT (main_window, button_enter, "button_enter");
  GLADE_HOOKUP_OBJECT (main_window, button_sign, "button_sign");
  GLADE_HOOKUP_OBJECT (main_window, button_MS, "button_MS");
  GLADE_HOOKUP_OBJECT (main_window, button_MR, "button_MR");
  GLADE_HOOKUP_OBJECT (main_window, button_Mplus, "button_Mplus");
  GLADE_HOOKUP_OBJECT (main_window, button_mult, "button_mult");
  GLADE_HOOKUP_OBJECT (main_window, statusbar, "statusbar");

  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);

  return main_window;
}
