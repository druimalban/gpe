/* gtk_usefull_funcs.c, by Michael Berg <mberg@nmt.edu>
 * Functions for some basic dialog boxes and pop-up windows
 * used for displaying program info and instructions
 * Also includes several functions that simplify common widget
 * creation and modification.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include "gtk_usefull_funcs.h"

#define TEXT_DISPLAY_SPACING 3
#define TEXT_DISPLAY_WIDTH  434
#define TEXT_DISPLAY_HEIGHT 420

#define BUTTON_WIDTH  128
#define BUTTON_HEIGHT 32



/* -------------------------------------------------------------------
 * This function creates a new button with the specified label (NULL 
 * for no label), width & height (-1 for default), and whether or not
 * the button is clickable (TRUE for yes, FALSE for no).
 * ------------------------------------------------------------------- */
GtkWidget *button_new_with_properties (const gchar *label, int width,
				       int height, int is_sensitive)
{
  GtkWidget *new_button;

  if (label && strlen (label))
    {
      new_button = gtk_button_new_with_label (label);
    }
  else
    {
      new_button = gtk_button_new ();
    }

  gtk_widget_set_usize (new_button, width, height);
  gtk_widget_set_sensitive (new_button, is_sensitive);

  return (new_button);
}


GtkWidget *text_new_with_scrollbars (GtkWidget *vbox, 
				     int text_width, int text_height, 
				     int vscroll, int hscroll, 
				     int editable)
{
  GtkWidget *table;
  GtkWidget *new_text_widget;
  GtkWidget *vscrollbar;
  GtkWidget *hscrollbar;

  /* Make a table with specified size to put everything in */
  table = gtk_table_new (2, 2, FALSE);
  gtk_widget_set_usize (table, text_width, text_height);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Make the text widget to return to caller */
  new_text_widget = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (new_text_widget), editable);

  /* Next line causes line wrap on word boundaries, so words
     aren't broken in the middle at the end of a line */
  gtk_text_set_word_wrap(GTK_TEXT (new_text_widget), TRUE);

  gtk_table_attach (GTK_TABLE (table), new_text_widget, 0, 1, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (new_text_widget);

  /* Make the vertical scrollbar, if one is needed */
  if (vscroll)
    {
      vscrollbar = gtk_vscrollbar_new (GTK_TEXT (new_text_widget)->vadj);
      gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
			GTK_FILL, 
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (vscrollbar);
    }

  /* Make the horizontal scrollbar, if one is needed */
  if (hscroll)
    {
      hscrollbar = gtk_hscrollbar_new (GTK_TEXT (new_text_widget)->hadj);
      gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 
			GTK_FILL, 0, 0);
      gtk_widget_show (hscrollbar);
    }

  return (new_text_widget);
}


/* -------------------------------------------------------------------
 * This function is called by the various dialog and pop-up windows
 * to destroy themselves when a certain button is clicked.
 * ------------------------------------------------------------------- */
void dismiss_window (GtkWidget *widget, gpointer window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}


/* -------------------------------------------------------------------
 * This function will open a dialog window with a given title and a
 * given text message.
 *
 * Useful for any error/warning or general purpose messages
 * ------------------------------------------------------------------- */
void display_ok_dialog (const gchar *title, GtkWidget *widget, 
			int widget_spacing,
			GtkSignalFunc extra_ok_func, gpointer data, 
			int resizable, int has_focus)
{
  GtkWidget *main_vbox;
  GtkWidget *dialog_window;
  GtkWidget *button;

  /* Make the display window */
  dialog_window = gtk_dialog_new ();
  gtk_window_set_policy (GTK_WINDOW (dialog_window), 
			 resizable, resizable, resizable);
  gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy", 
		      GTK_SIGNAL_FUNC (dismiss_window), dialog_window);
  gtk_window_set_title (GTK_WINDOW (dialog_window), title);

  /* Make the main vbox to hold all other stuff */
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), widget_spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
		      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (main_vbox), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  /* OK button - dismisses this window */
  button = button_new_with_properties (" Ok ", BUTTON_WIDTH, 
				       BUTTON_HEIGHT, TRUE);

  if (extra_ok_func)
    {
      gtk_signal_connect (GTK_OBJECT (button), "clicked", 
			  GTK_SIGNAL_FUNC (extra_ok_func), data);
    }
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dismiss_window), dialog_window);
  /* allow it as default button */
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
		      button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);  /* make button the default */
  gtk_widget_show (button);

  /* Make everything visible at once */
  gtk_widget_show (dialog_window);

  /* If has_focus is TRUE, make this window the one with the focus, 
     don't allow action in parent window until this one closes */
  if (has_focus)
    {
      gtk_grab_add (dialog_window);
    }
}


/* -------------------------------------------------------------------
 * This function will open a yes/no dialog window with a given title 
 * and a given text message.
 *
 * Useful for any yes/no or option approval/rejection messages
 * ------------------------------------------------------------------- */
void display_yes_no_dialog (const gchar *title, GtkWidget *widget, 
			    int widget_spacing,
			    GtkSignalFunc yes_func, gpointer yes_data,
			    GtkSignalFunc no_func, gpointer no_data,
			    int resizable, int has_focus)
{
  GtkWidget *dialog_window;
  GtkWidget *main_vbox;
  GtkWidget *button;

  /* Make the display window */
  dialog_window = gtk_dialog_new ();
  gtk_window_set_policy (GTK_WINDOW (dialog_window), 
			 resizable, resizable, resizable);
  gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy", 
		      GTK_SIGNAL_FUNC (dismiss_window), dialog_window);
  gtk_window_set_title (GTK_WINDOW (dialog_window), title);

  /* Make the main vbox to hold all other stuff */
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (main_vbox), widget_spacing);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
		      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  gtk_box_pack_start (GTK_BOX (main_vbox), widget, TRUE, TRUE, 0);
  gtk_widget_show (widget);

  /* Yes button */
  button = button_new_with_properties (" Yes ", BUTTON_WIDTH, 
				       BUTTON_HEIGHT, TRUE);
  if (yes_func)
    {
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (yes_func), yes_data);
    }
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dismiss_window), dialog_window);

  /* allow it as default button */
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
		      button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* No button */
  button = button_new_with_properties (" No ", BUTTON_WIDTH, 
				       BUTTON_HEIGHT, TRUE);
  if (no_func)
    {
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (no_func), no_data);
    }
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (dismiss_window), dialog_window);

  /* allow it as default button */
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
		      button, FALSE, FALSE, 0);
  gtk_widget_grab_default (button);  /* make button the default */
  gtk_widget_show (button);

  /* Make everything visible at once */
  gtk_widget_show (dialog_window);

  /* If has_focus is TRUE, make this window the one with the focus, 
     don't allow action in parent window until this one closes */
  if (has_focus)
    {
      gtk_grab_add (dialog_window);
    }
}


/* -------------------------------------------------------------------
 * This function will open a read-only text window with a given title
 * and then read in a specified file to display in that window.
 *
 * Useful for HOWTO/Instruction windows or a simple file viewer
 * ------------------------------------------------------------------- */
void display_text_file (const gchar *title, const gchar *file_to_open,
			GtkSignalFunc extra_ok_func, gpointer ok_data)
{
  FILE *opened_file;
  char buffer[1024];
  int chars_read;

  GtkWidget *vbox;
  GtkWidget *text_widget;


  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);

  text_widget = text_new_with_scrollbars (vbox, TEXT_DISPLAY_WIDTH, 
					  TEXT_DISPLAY_HEIGHT, 
					  TRUE, FALSE, FALSE);

  /* Read the instruction file into the text window */
  if ((opened_file = fopen(file_to_open, "r")) == NULL)
    {
      fprintf(stderr, "Error: couldn't open file \"%s\"\n", file_to_open);
    }
  else
    {
      /* Read in the instructions and put them in the text display widget */
      gtk_text_freeze (GTK_TEXT (text_widget));
      while ( (chars_read = fread (buffer, sizeof(char), 1024, opened_file)) )
	{
	  gtk_text_insert (GTK_TEXT (text_widget), NULL, NULL, NULL, 
			   buffer, chars_read);
        }
      gtk_text_thaw (GTK_TEXT (text_widget));

      fclose(opened_file);
    }

  /* Put the the text widget stuff in a pop-up dialog window */
  display_ok_dialog (title, vbox, TEXT_DISPLAY_SPACING,
		     extra_ok_func, ok_data, TRUE, FALSE);
}


/* -------------------------------------------------------------------
 * The following group of functions simplify the creation of menues
 * while providing some features not currently available from the
 * ItemFactory method of making menues.
 * ------------------------------------------------------------------- */

GtkWidget *create_submenu (GtkWidget *menu_bar, gchar *label, 
			   int right_justified)
{
  GtkWidget *menu_item;
  GtkWidget *submenu;

  menu_item = gtk_menu_item_new_with_label (label);
  gtk_menu_bar_append (GTK_MENU_BAR (menu_bar), menu_item);

  if (right_justified)
    {
      gtk_menu_item_right_justify (GTK_MENU_ITEM (menu_item));
    }

  gtk_widget_show (menu_item);

  submenu = gtk_menu_new ();
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);

  return (submenu);
}


GtkWidget *create_menu_item (GtkWidget *menu, gchar *label, 
			     GtkSignalFunc func, gpointer data)
{
  GtkWidget *menu_item;

  if (label && (strlen (label) > 0))
    {
      menu_item = gtk_menu_item_new_with_label (label);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (func), data);
    }
  else
    {
      menu_item = gtk_menu_item_new ();
    }

  gtk_menu_append (GTK_MENU (menu), menu_item);

  return (menu_item);
}


GtkWidget *create_menu_check (GtkWidget *menu, gchar *label, 
			      GtkSignalFunc func, gpointer data)
{
    GtkWidget *menu_check;

    menu_check = gtk_check_menu_item_new_with_label (label);
    gtk_signal_connect (GTK_OBJECT (menu_check), "toggled",
			GTK_SIGNAL_FUNC (func), data);

    gtk_menu_append (GTK_MENU (menu), menu_check);

    return (menu_check);
}


GtkWidget *create_menu_radio (GtkWidget *menu, gchar *label, GSList **group,
			      GtkSignalFunc func, gpointer data)
{
  GtkWidget *menu_radio;

  menu_radio = gtk_radio_menu_item_new_with_label (*group, label);
  *group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_radio));
  gtk_signal_connect (GTK_OBJECT (menu_radio), "toggled",
		      GTK_SIGNAL_FUNC (func), data);

  gtk_menu_append (GTK_MENU (menu), menu_radio);

  return (menu_radio);
}


