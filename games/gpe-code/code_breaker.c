/* Code Breaker, by Michael Berg <mberg@nmt.edu>
 * a version of MasterMind (TM) written with GTK+
 */

/* Copyright (C) 1999, Michael Berg */
/* This program is released under the GPL and is freely distributable */

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>

#include "gtk_style_funcs.h"
#include "gtk_timer.h"
#include "gtk_usefull_funcs.h"

#include "gtk_graphic_funcs.h"
#include "game_pixmaps.h"

#include "usefull_funcs.h"

/* Define some program infomation and the game board size */
#define APP_NAME "Code Breaker"
#define VERSION_NUM "1.2.0"
#define AUTHOR  "Michael Berg"
#define AUTHOR_EMAIL "mberg@nmt.edu"

#define GAME_ROWS 10
#define GAME_COLS 4
#define RESULT_COLS 2

#define COLOR_ROWS 4
#define COLOR_COLS 2

#define SPACING 2
#define MESSAGE_SPACING 20

#define BUTTON_WIDTH  24 
#define BUTTON_HEIGHT 16 

#define _(x) gettext(x)

/* Define the colors to use for the codes */
GdkColor color_black  = {0, 0x0000, 0x0000, 0x0000};
GdkColor color_white  = {0, 0xffff, 0xffff, 0xffff};
GdkColor color_red    = {0, 0xffff, 0x0000, 0x0000};
GdkColor color_blue   = {0, 0x0000, 0x0000, 0xffff};
GdkColor color_yellow = {0, 0xffff, 0xffff, 0x0000};
GdkColor color_green  = {0, 0x0000, 0xeaff, 0x0000};
GdkColor color_cyan   = {0, 0x66ff, 0xffff, 0xffff};
GdkColor color_gray   = {0, 0x82ff, 0x82ff, 0x82ff};

/* As long as NUM_COLORS is the last element, it will be the number of 
   colors in this enumerated type.  Use it for giving the size of the 
   color index arrays */
enum {DEFAULT, BLACK, WHITE, RED, BLUE, YELLOW, 
      GREEN, CYAN, GRAY, NUM_COLORS};

GtkStyle *style[NUM_COLORS];


/* Data structure definition for all the buttons and display widgets */
typedef struct {
  GtkWidget *widget;
  int value;   /* used for color or number storage */
} widget_data;

/* Data structure for storing all the game data */
typedef struct {
  GtkWidget *main_app_window;
  widget_data guess_grid[GAME_ROWS][GAME_COLS];
  widget_data code_grid[1][GAME_COLS];
  widget_data color_grid[COLOR_ROWS][COLOR_COLS];
  widget_data result_grid[GAME_ROWS][RESULT_COLS];
  type_timer  game_timer;
  int game_in_progress;
  int current_row;
  int active_skill_level;
} game_data;


/* Forward declarations of subroutines */
void new_game_dialog (GtkWidget *widget, game_data *board);
void start_new_game (GtkWidget *widget, game_data *board);

void reset_game_area (widget_data game_area[][GAME_COLS], int num_rows);
void reset_result_area (widget_data result_area[GAME_ROWS][RESULT_COLS]);
void reset_color_area (GtkWidget *widget, game_data *board);

void row_set_sensitive (widget_data button_array[][GAME_COLS], 
			int row_to_set, int sensitive);

void generate_new_code (widget_data code_array[1][GAME_COLS], 
			int num_rows, int num_cols, int active_skill_level);
void check_code_guess (GtkWidget *widget, game_data *board);
void reveal_code (widget_data code[1][GAME_COLS]);

void set_game_in_progress (GtkWidget *widget, game_data *board);
void set_skill_level (GtkWidget *widget, gpointer num_colors);
void set_current_color (GtkWidget *widget, gpointer int_color);
void set_guess_color (GtkWidget *widget, widget_data *guess_data);

void about (void);

void player_wins_message (game_data *board);
void player_loses_message (game_data *board);


/* Global Variables for game state */
volatile widget_data current_color;
volatile int skill_level = 6;


/* -----------------------------------------------------------------
 *   Start the Actual Program Now
 * ----------------------------------------------------------------- */

int main (int argc, char *argv[])
{
  GtkWidget *main_window;
  GtkWidget *main_vbox;
  GtkWidget *menubar;
  GtkWidget *submenu;
  GtkWidget *menu_item;

  GtkWidget *color_vbox;
  GtkWidget *vbox;

  GtkWidget *main_table;
  GtkWidget *key_table;
  GtkWidget *code_table;
  GtkWidget *result_table;
  GtkWidget *guess_table;
  GtkWidget *color_table;

  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;

  GSList *skill_group = NULL;

  int row, col;
  int color;

  setlocale (LC_ALL, "");
  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (PACKAGE, "UTF-8");
  textdomain (PACKAGE);

  /* Store all the game data in here */
  game_data game_board;


  /* Process any gtk related command line arguments */
  gtk_init (&argc, &argv);

  /* Allocate all the colors this program uses, just to be safe */
  gdk_color_alloc (gdk_colormap_get_system (), &color_black);
  gdk_color_alloc (gdk_colormap_get_system (), &color_white);
  gdk_color_alloc (gdk_colormap_get_system (), &color_red);
  gdk_color_alloc (gdk_colormap_get_system (), &color_blue);
  gdk_color_alloc (gdk_colormap_get_system (), &color_yellow);
  gdk_color_alloc (gdk_colormap_get_system (), &color_green);
  gdk_color_alloc (gdk_colormap_get_system (), &color_cyan);
  gdk_color_alloc (gdk_colormap_get_system (), &color_gray);

  /* Now create the color styles that the buttons can take on, using
     the enumerated values as index for easy lookup later */
  style[DEFAULT] = get_default_style ();
  style[BLACK]   = create_new_style (color_black);
  style[WHITE]   = create_new_style (color_white);
  style[RED]     = create_new_style (color_red);
  style[BLUE]    = create_new_style (color_blue);
  style[YELLOW]  = create_new_style (color_yellow);
  style[GREEN]   = create_new_style (color_green);
  style[CYAN]    = create_new_style (color_cyan);
  style[GRAY]    = create_new_style (color_gray);


  /* ------------------------------------------------------------
   * Make all the GUI elements and piece together the game window
   * ------------------------------------------------------------ */

  /* Create main window, connect destroy event, and set window's title */
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  game_board.main_app_window = main_window;
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy", 
                      GTK_SIGNAL_FUNC (gtk_main_quit), "quit");
  gtk_window_set_title (GTK_WINDOW(main_window), APP_NAME " - by " AUTHOR);

  /* Don't allow the window to be resized by user */
  gtk_window_set_policy (GTK_WINDOW (main_window), FALSE, FALSE, FALSE);

  /* Make the main vbox to put everything in */
  main_vbox = gtk_vbox_new (FALSE, SPACING);
  gtk_container_add (GTK_CONTAINER (main_window), main_vbox);
  gtk_widget_show (main_vbox);

  /* ------------------------------------------------------------
   * Make the menu bar at top of game window 
   * ------------------------------------------------------------ */
  
  menubar = gtk_menu_bar_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);

  /* "Game" pulldown menu */
  submenu = create_submenu (menubar, _("Game"), FALSE);

  menu_item = create_menu_item (submenu, _("New Game"), 
				new_game_dialog, &game_board);
  gtk_widget_show (menu_item);

  menu_item = create_menu_item (submenu, NULL, NULL, NULL);
  gtk_widget_show (menu_item);

  menu_item = create_menu_item (submenu, _("Quit"), gtk_main_quit, NULL);
  gtk_widget_show (menu_item);

  /* "Skill Level" pulldown menu */
  submenu = create_submenu (menubar, _("Skill Level"), FALSE);

  menu_item = create_menu_radio (submenu, _("Beginner (6 colors)"), 
				 &skill_group,
				 set_skill_level, (gpointer) 6);
  gtk_signal_connect (GTK_OBJECT (menu_item), "toggled",
		      GTK_SIGNAL_FUNC (reset_color_area), &game_board);
  gtk_widget_show (menu_item);

  menu_item = create_menu_radio (submenu, _("Intermediate (7 colors)"), 
				 &skill_group,
				 set_skill_level, (gpointer) 7);
  gtk_signal_connect (GTK_OBJECT (menu_item), "toggled",
		      GTK_SIGNAL_FUNC (reset_color_area), &game_board);
  gtk_widget_show (menu_item);

  menu_item = create_menu_radio (submenu, _("Advanced (8 colors)"), 
				 &skill_group,
				 set_skill_level, (gpointer) 8);
  gtk_signal_connect (GTK_OBJECT (menu_item), "toggled",
		      GTK_SIGNAL_FUNC (reset_color_area), &game_board);
  gtk_widget_show (menu_item);

  /* "Help" pulldown menu */
  submenu = create_submenu (menubar, _("Help"), TRUE);

  menu_item = create_menu_item (submenu, _("About"), about, NULL);
  gtk_widget_show (menu_item);

  /* ------------------------------------------------------------
   * Done making menu bar 
   * ------------------------------------------------------------ */


  /* Create a basic placement grid (non-homogenous) for the 
     different game areas to be put in */
  main_table = gtk_table_new(2, 3, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(main_table), SPACING);
  gtk_table_set_col_spacings(GTK_TABLE(main_table), SPACING);
  gtk_container_border_width (GTK_CONTAINER (main_table), (SPACING * 2));
  gtk_box_pack_start (GTK_BOX (main_vbox), main_table, TRUE, TRUE, 0);
  gtk_widget_show (main_table);


  /* -------------------------------------
   * Now make each of the major game areas
   * ------------------------------------- */

  /* Key area above result area --------------------------------- */
  frame = gtk_frame_new(_("Key"));
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 0, 1, 0, 1);
  gtk_widget_show (frame);

  key_table = gtk_table_new (1, 2, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (key_table), SPACING);
  gtk_table_set_col_spacings (GTK_TABLE (key_table), SPACING);
  gtk_container_border_width (GTK_CONTAINER (key_table), (SPACING * 2));
  gtk_container_add (GTK_CONTAINER (frame), key_table);
  gtk_widget_show (key_table);

  /* Black Key Button */
  button = button_new_with_properties (NULL, BUTTON_HEIGHT, 
				       BUTTON_HEIGHT, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (key_table), button, 0, 1, 0, 1);
  gtk_widget_set_style (button, style[BLACK]);
  gtk_widget_show (button);

  /* White Key Button */
  button = button_new_with_properties (NULL, BUTTON_HEIGHT, 
				       BUTTON_HEIGHT, FALSE);
  gtk_table_attach_defaults (GTK_TABLE (key_table), button, 1, 2, 0, 1);
  gtk_widget_set_style (button, style[WHITE]);
  gtk_widget_show (button);


  /* Secret Code area ------------------------------------------- */
  frame = gtk_frame_new(_("Crack this Code"));
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 1, 2, 0, 1);
  gtk_widget_show (frame);

  code_table = gtk_table_new (1, GAME_COLS, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (code_table), SPACING);
  gtk_table_set_col_spacings (GTK_TABLE (code_table), SPACING);
  gtk_container_border_width (GTK_CONTAINER (code_table), (SPACING * 2));
  gtk_container_add (GTK_CONTAINER (frame), code_table);
  gtk_widget_show (code_table);

  /* Make all the secret code buttons */
  for (col = 0; col < GAME_COLS; col++)
    {
      button = button_new_with_properties ("?", BUTTON_WIDTH, 
					   BUTTON_HEIGHT, FALSE);
      game_board.code_grid[0][col].widget = button;
      gtk_table_attach_defaults (GTK_TABLE (code_table), button, 
				 col, col + 1, 0, 1);
      gtk_widget_show (button);
    }


  /* Timer area ------------------------------------------------- */
  frame = gtk_frame_new(_("Timer"));
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 2, 3, 0, 1);
  gtk_widget_show (frame);

  label = gtk_label_new ("");
  gtk_container_add (GTK_CONTAINER (frame), label);
  initialize_timer (&game_board.game_timer, label, _("seconds"));
  gtk_widget_show (label);

  /* Result area - below Key area ------------------------------- */
  frame = gtk_frame_new(NULL);
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 0, 1, 1, 2);
  gtk_widget_show (frame);

  result_table = gtk_table_new (GAME_ROWS, RESULT_COLS, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (result_table), SPACING);
  gtk_table_set_col_spacings (GTK_TABLE (result_table), SPACING);
  gtk_container_border_width (GTK_CONTAINER (result_table), (SPACING * 2));
  gtk_container_add (GTK_CONTAINER (frame), result_table);
  gtk_widget_show (result_table);

  /* Make all the result widgets */
  for (row = 0; row < GAME_ROWS; row++)
    {
      for (col = 0; col < RESULT_COLS; col++)
	{
	  frame = gtk_frame_new (NULL);
	  gtk_table_attach_defaults (GTK_TABLE (result_table), frame, 
				     col, col + 1, row, row + 1);
	  gtk_widget_set_usize (frame, BUTTON_HEIGHT, BUTTON_HEIGHT);
	  gtk_widget_set_style (frame, style[GRAY]);
	  gtk_widget_show (frame);

	  label = gtk_label_new (" ");  /* blank to start with */
	  game_board.result_grid[row][col].widget = label;
	  gtk_container_add (GTK_CONTAINER (frame), label);
	  gtk_widget_show (label);
	}
    }


  /* Guessing area - below Master Code area --------------------- */
  frame = gtk_frame_new(NULL);
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 1, 2, 1, 2);
  gtk_widget_show (frame);

  guess_table = gtk_table_new (GAME_ROWS, GAME_COLS, TRUE);
  gtk_table_set_row_spacings (GTK_TABLE (guess_table), SPACING);
  gtk_table_set_col_spacings (GTK_TABLE (guess_table), SPACING);
  gtk_container_border_width (GTK_CONTAINER (guess_table), (SPACING * 2));
  gtk_container_add (GTK_CONTAINER (frame), guess_table);
  gtk_widget_show (guess_table);

  /* Make all the guessing buttons */
  for (row = 0; row < GAME_ROWS; row++)
    {
      for (col = 0; col < GAME_COLS; col++)
	{
	  button = button_new_with_properties (NULL, BUTTON_WIDTH, 
					       BUTTON_HEIGHT, TRUE);
	  game_board.guess_grid[row][col].widget = button;

	  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			     GTK_SIGNAL_FUNC(set_guess_color), 
			     &(game_board).guess_grid[row][col]);

	  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			     GTK_SIGNAL_FUNC(set_game_in_progress), 
			     &game_board);

	  gtk_table_attach_defaults (GTK_TABLE (guess_table), button, 
				     col, col + 1, row, row + 1);
	  gtk_widget_show (button);
	}
    }


  /* Multipurpose vbox to put rest of components in ------------- */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_table_attach_defaults(GTK_TABLE(main_table), vbox, 2, 3, 1, 2);
  gtk_widget_show (vbox);

  /* Color selector palete frame */
  frame = gtk_frame_new(_("Colors"));
  gtk_container_border_width (GTK_CONTAINER (frame), SPACING);
  gtk_box_pack_end (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  color_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (frame), color_vbox);
  gtk_widget_show (color_vbox);

  color_table = gtk_table_new (COLOR_ROWS, COLOR_COLS, TRUE);
  gtk_container_border_width (GTK_CONTAINER (color_table), (SPACING * 2));
  gtk_table_set_row_spacings (GTK_TABLE (color_table), SPACING);
  gtk_table_set_col_spacings (GTK_TABLE (color_table), SPACING);
  gtk_box_pack_start (GTK_BOX (color_vbox), color_table, FALSE, FALSE, 0);
  gtk_widget_show (color_table);

  /* Make the color selection buttons */
  color = (int) BLACK;
  for (row = 0; row < COLOR_ROWS; row++)
    {
      for (col = 0; col < COLOR_COLS; col++, color++)
	{
	  button = button_new_with_properties (NULL, BUTTON_WIDTH, 
					       BUTTON_HEIGHT, TRUE);
	  game_board.color_grid[row][col].widget = button;
	  game_board.color_grid[row][col].value = color;

	  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
			     GTK_SIGNAL_FUNC(set_current_color), 
			     (gpointer) color);
	  gtk_table_attach_defaults (GTK_TABLE (color_table), button, 
				     col, col + 1, row, row + 1);
	  gtk_widget_show (button);

	  gtk_widget_set_style (game_board.color_grid[row][col].widget, 
				style[game_board.color_grid[row][col].value]);
	}
    }

  /* Current color display field */
  button = gtk_toggle_button_new ();
  current_color.widget = button;

  gtk_container_border_width (GTK_CONTAINER (button), (SPACING * 2));
  gtk_box_pack_start (GTK_BOX (color_vbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, BUTTON_WIDTH, BUTTON_HEIGHT);
  gtk_widget_set_sensitive(button, FALSE);  /* don't let anyone click on it */
  gtk_widget_show (button);


  /* "Guess" button */
  button = button_new_with_properties(_(" Guess "), -1, 
				      BUTTON_HEIGHT + 12, TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(check_code_guess), &game_board);
  gtk_container_border_width (GTK_CONTAINER (button), SPACING);
  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, SPACING);
  gtk_widget_show (button);
  

  /* "New Game" button */
  button = button_new_with_properties(_("New Game"), -1, 
				      BUTTON_HEIGHT + 12, TRUE);
  gtk_signal_connect(GTK_OBJECT(button), "clicked", 
		     GTK_SIGNAL_FUNC(new_game_dialog), &game_board);
  gtk_container_border_width (GTK_CONTAINER (button), SPACING);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, SPACING);
  gtk_widget_show (button);



  /* ------------------------------
   * Done making all the game areas
   * ------------------------------ */

  /* Make everything appear on screen at once */
  gtk_widget_show (main_window);

  /* Now generate an initial code to start off the game */
  start_new_game (main_window, &game_board);

  /* Let GTK+ take over now */
  gtk_main ();

  return (0);
}



/* -------------------------------------------
 * Here are all the subroutine implementations
 * ------------------------------------------- */

void new_game_dialog (GtkWidget *widget, game_data *board)
{
  GtkWidget *hbox;
  GtkWidget *message;
  GtkWidget *question_pixmap;

  if (board->game_in_progress)
    {
      /* hbox to put message components in */
      hbox = gtk_hbox_new (FALSE, SPACING);
      gtk_widget_show (hbox);

      /* Make the question pixmap and put it in the hbox */
      question_pixmap = widget_new_from_xpm (board->main_app_window, 
					     (gchar **) question_xpm);
      gtk_widget_show (question_pixmap);
      gtk_box_pack_start (GTK_BOX (hbox), question_pixmap, FALSE, FALSE, 0);

      /* Make the question message and put it in the hbox */
      message = gtk_label_new (_("Do you really want to quit this\n"
			       "round and start a new game?"));
      gtk_widget_show (message);
      gtk_box_pack_start (GTK_BOX (hbox), message, FALSE, FALSE, 0);

      /* Display everything is a popup dialog box */
      display_yes_no_dialog (_("Start New Game?"), hbox, MESSAGE_SPACING,
			     start_new_game, board, 
			     NULL, NULL, 
			     FALSE, TRUE);
    }
  else
    {
      start_new_game (widget, board);
    }
}


void start_new_game (GtkWidget *widget, game_data *board)
{
  /* Stop timer is here for if user reset in middle of previous round */
  board->game_in_progress = FALSE;
  stop_timer (&board->game_timer);
  reset_timer (&board->game_timer, 0);

  board->current_row = (GAME_ROWS - 1);

  reset_game_area (board->guess_grid, GAME_ROWS);
  reset_game_area (board->code_grid, 1);
  reset_result_area (board->result_grid);

  /* This function also generates a new color code */
  reset_color_area (NULL, board);

  row_set_sensitive (board->guess_grid, board->current_row, TRUE);
}


void reset_game_area (widget_data game_area[][GAME_COLS], int num_rows)
{
  int row, col;

  for (row = 0; row < num_rows; row++)
    {
      for (col = 0; col < GAME_COLS; col++)
	{
	  game_area[row][col].value = DEFAULT;
	  gtk_widget_set_style (game_area[row][col].widget, style[DEFAULT]);
	  gtk_widget_set_sensitive (game_area[row][col].widget, FALSE);
	}
    }
}


void reset_result_area (widget_data result_area[GAME_ROWS][RESULT_COLS])
{
  int row, col;

  for (row = 0; row < GAME_ROWS; row++)
    {
      for (col = 0; col < RESULT_COLS; col++)
	{
	  result_area[row][col].value = 0;
	  gtk_label_set (GTK_LABEL (result_area[row][col].widget), " ");
	}
    }
}


/* This function uses the global variables "skill_level" and 
   "current_color" */
void reset_color_area (GtkWidget *widget, game_data *board)
{
  GtkWidget *message_label;
  gchar message[100];
  int row, col, i = 0;

  if (board->game_in_progress)
    {
      if (GTK_CHECK_MENU_ITEM (widget)->active)
	{
	  sprintf (message, _("When you start a New Game, %d colors\n"
		   "will be available to you and in the code"), skill_level);
	  message_label = gtk_label_new (message);
	  display_ok_dialog (_("Skill Level"), message_label, MESSAGE_SPACING, 
			     NULL, NULL, FALSE, TRUE);
	}
    }
  else
    {
      board->active_skill_level = skill_level;

      /* Make only the number of colors used in code visible */
      for (row = 0; row < COLOR_ROWS; row++)
	{
	  for (col = 0; col < COLOR_COLS; col++, i++)
	    {
	      if (i < skill_level)
		{
		  gtk_widget_show (board->color_grid[row][col].widget);
		}
	      else
		{
		  gtk_widget_hide (board->color_grid[row][col].widget);
		}
	    }
	}

      /* Set current color display window to BLACK as the default */
      current_color.value = BLACK;
      gtk_widget_set_style (current_color.widget, 
			    style[ current_color.value ]);

      /* And generate a new code to make sure that any colors removed from 
	 the Color area are not in the code. That would be cheating!! */
      generate_new_code (board->code_grid, 1, GAME_COLS, 
			 board->active_skill_level);

      /* Uncomment the following line for verifying the new code is ok */
      /* reveal_code (board->code_grid); */
    }
}


void row_set_sensitive (widget_data button_array[][GAME_COLS],
			int row_to_set, int sensitive)
{
  int col;

  for (col = 0; col < GAME_COLS; col++)
    {
      gtk_widget_set_sensitive (button_array[row_to_set][col].widget, 
				sensitive);
    }
}


void generate_new_code (widget_data code_array[1][GAME_COLS], 
			int num_rows, int num_cols, int active_skill_level)
{
  int row, col;

  for (row = 0; row < num_rows; row++)
    {
      for (col = 0; col < num_cols; col++)
	{
	  code_array[0][col].value = rand_from_to (1, active_skill_level);
	}
    }
}


void reveal_code (widget_data code[1][GAME_COLS])
{
  int i;
  for (i = 0; i < GAME_COLS; i++)
    {
      gtk_widget_set_style (code[0][i].widget, style[ code[0][i].value ]);
    }
}


void set_game_in_progress (GtkWidget *widget, game_data *board)
{
  board->game_in_progress = TRUE;
}


/* This function modifies the global variable "skill_level" */
void set_skill_level (GtkWidget *widget, gpointer num_colors)
{
  if (GTK_CHECK_MENU_ITEM (widget)->active)
    {
      skill_level = (int) num_colors;
    }
}


/* This function modifies the global variable "current_color" */
void set_current_color (GtkWidget *widget, gpointer int_color)
{
  current_color.value = (int) int_color;
  gtk_widget_set_style (current_color.widget, style[ current_color.value ]);
}


/* This function accesses the global variable "current_color" */
void set_guess_color (GtkWidget *widget, widget_data *guess_data )
{
  gtk_widget_set_style (guess_data->widget, style[ current_color.value ]);
  guess_data->value = current_color.value;
}


void check_code_guess (GtkWidget *widget, game_data *board)
{
  GtkWidget *message;
  char result[10];
  int used_code[GAME_COLS];
  int used_guess[GAME_COLS];
  int code_col, guess_col;
  int code_not_complete = FALSE;
  int rc_rp = 0, rc_wp = 0;


  /* Initialize calculation arrays */
  for (code_col = 0; code_col < GAME_COLS; code_col++)
    {
      used_code[code_col] = FALSE;
      used_guess[code_col] = FALSE;
    }

  /* First make sure that all guess elements in row have been 
     assigned a color by the player */
  for (guess_col = 0; guess_col < GAME_COLS; guess_col++)
    {
      if (board->guess_grid[board->current_row][guess_col].value == DEFAULT)
	{
	  code_not_complete = TRUE;
	}
    }

  if (code_not_complete)
    {
      message = gtk_label_new (_("Your code is not complete yet!\n\n"
			       "Assigned a color to each button in\n"
			       "this row and click \"Guess\" again"));
      display_ok_dialog (_("Warning"), message, MESSAGE_SPACING, 
			 NULL, NULL, FALSE, TRUE);
    }

  /* If code is complete then start comparing players guess to the 
     secret code */
  else
    {
      /* If this is the first guess, timer hasn't started yet, so start it */
      if (board->current_row == (GAME_ROWS - 1))
	{
	  start_timer (&board->game_timer, 1000);
	}

      /* Don't allow any further changes to this guess row */
      row_set_sensitive (board->guess_grid, board->current_row, FALSE);  

      /* First check for right color and in right position elements */
      for (code_col = 0; code_col < GAME_COLS; code_col++)
	{
	  if (board->code_grid[0][code_col].value == 
	      board->guess_grid[board->current_row][code_col].value)
	    {
	      used_code[code_col] = TRUE;
	      used_guess[code_col] = TRUE;
	      rc_rp++;
	    }
	}

      /* Next check for right color but in wrong position elements */
      for (code_col = 0; code_col < GAME_COLS; code_col++)
	{
	  if (used_code[code_col] == FALSE)
	    {
	      for (guess_col = 0; (guess_col < GAME_COLS) && 
		     (used_code[code_col] == FALSE); guess_col++)
		{
		  if ((used_guess[guess_col] == FALSE) && 
		      (board->code_grid[0][code_col].value == 
		       board->guess_grid[board->current_row][guess_col].value))
		    {
		      used_code[code_col] = TRUE;
		      used_guess[guess_col] = TRUE;
		      rc_wp++;
		    }
		}
	    }
	}

      /* And put the results from comparison in result grid for user to see */
      sprintf(result, "%d", rc_rp);
      gtk_label_set (GTK_LABEL (board->result_grid[board->
						  current_row][0].widget), 
		     result);
 
      sprintf(result, "%d", rc_wp);
      gtk_label_set (GTK_LABEL (board->result_grid[board->
						  current_row][1].widget), 
		     result);


      if (board->game_in_progress)
	{
	  /* Check for winner */
	  if (rc_rp == GAME_COLS)
	    {
	      stop_timer (&board->game_timer);
	      board->game_in_progress = FALSE;
	      player_wins_message (board);
	    }

	  /* If players turns are not all used up, and the player hasn't 
	     cracked the secret code, advance to the next row so the 
	     player can make another guess at the secret code */
	  else if (board->current_row > 0)
	    {
	      board->current_row--;
	      row_set_sensitive (board->guess_grid, board->current_row, TRUE);
	    }

	  /* Player must be losser (only option left) */
	  else
	    {
	      stop_timer (&board->game_timer);
	      board->game_in_progress = FALSE;
	      player_loses_message (board);
	    }
	}

    }
}


void about (void)
{
  GtkWidget *message;

  message = gtk_label_new 
    (APP_NAME " " VERSION_NUM "  --  Author: " AUTHOR "\n"
     "\n"
     "A logic game where you try to crack the color code\n"
     "\n"
     "Copyright (C) 1999, " AUTHOR "\n"
     "\n"
     "This program is released under the General Public\n"
     "Licence and is freely distributable\n"
     "\n"
     "\n"
     "Send comments, suggestions, and bug reports to\n"
     AUTHOR_EMAIL);

  display_ok_dialog (APP_NAME ": About", message, MESSAGE_SPACING, 
		     NULL, NULL, FALSE, FALSE);
}


void player_wins_message (game_data *board)
{
  GtkWidget *hbox;
  GtkWidget *winner_pixmap;
  GtkWidget *message;
  int player_guesses;
  int timer_count;
  char skill_string[80];
  char message_string[400];

  hbox = gtk_hbox_new (FALSE, SPACING);
  gtk_widget_show (hbox);

  /* Make the winner pixmap and put it in the hbox */
  winner_pixmap = widget_new_from_xpm (board->main_app_window, 
				       (gchar **) winner_xpm);
  gtk_widget_show (winner_pixmap);
  gtk_box_pack_start (GTK_BOX (hbox), winner_pixmap, FALSE, FALSE, 0);

  /* Make the winner message and put it in the hbox */
  player_guesses = GAME_ROWS - (board->current_row);
  timer_count = timer_current_value (&board->game_timer);

  if (board->active_skill_level == 8)
    {
      sprintf (skill_string, _("Advanced"));
    }
  else if (board->active_skill_level == 7)
    {
      sprintf (skill_string, _("Intermediate"));
    }
  else
    {
      sprintf (skill_string, _("Beginner"));
    }

  sprintf (message_string, _("Congratulations, you cracked the %s\n"
	   "code in %d seconds with %d guesses"), 
	   skill_string, timer_count, player_guesses);

  message = gtk_label_new (message_string);
  gtk_widget_show (message);

  gtk_box_pack_start (GTK_BOX (hbox), message, FALSE, FALSE, 0);

  /* Display everthing in a nice popup dialog window */
  display_ok_dialog (_("We Have a Winner!!"), hbox, MESSAGE_SPACING, 
		     NULL, NULL, FALSE, TRUE);

  /* Reveal the secret code for the players inspection */
  reveal_code (board->code_grid);
}


void player_loses_message (game_data *board)
{
  GtkWidget *hbox;
  GtkWidget *message;
  GtkWidget *loser_pixmap;

  hbox = gtk_hbox_new (FALSE, SPACING);
  gtk_widget_show (hbox);

  /* Make the loser pixmap and put it in the hbox */  
  loser_pixmap = widget_new_from_xpm (board->main_app_window, 
				      (gchar **) loser_xpm);
  gtk_widget_show (loser_pixmap);
  gtk_box_pack_start (GTK_BOX (hbox), loser_pixmap, FALSE, FALSE, 0);

  /* Make the loser message and put it in the hbox */
  message = gtk_label_new (_("Sorry, but you didn't crack the code this time\n"
			   "The secret code has been\n"
			   "revealed for your inspection"));
  gtk_widget_show (message);

  gtk_box_pack_start (GTK_BOX (hbox), message, FALSE, FALSE, 0);

  /* Display everthing in a nice popup dialog window */
  display_ok_dialog (_("Too Bad - You Lose!"), hbox, MESSAGE_SPACING, 
		     NULL, NULL, FALSE, TRUE);

  /* Reveal the secret code for the players inspection */
  reveal_code (board->code_grid);
}

