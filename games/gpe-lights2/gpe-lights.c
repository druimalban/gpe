/* 
 *   GLOP: Gnomified Lights Out Puzzles
 *   Copyright (C) 2001 Andreas T. Hagli <athagl00@grm.hia.no>
 * 
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <libintl.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define _(x)  gettext(x)

#define LIGHT_SIZE 40
#define MAX_WIDTH 12
#define MAX_HEIGHT 9
#define GAME_EVENTS (GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK| GDK_BUTTON_RELEASE_MASK)

enum game_types { CLASSIC, TWOK, DELUX, KEYCHAIN, CUBE, MERLIN };
enum light_state { NO, ON, HALF, OFF };

static GtkWidget *moves_label, *lights_off, *lights_label;
static GtkWidget *vb, *status_box;
static GtkWidget *align, *draw_area;
static GtkWidget *pref_dialog;
static GdkPixbuf *lights;

static gchar *fname;

static int x_dir[8] = { 1, 1, 0,-1,-1,-1,0,1 };
static int y_dir[8] = { 0,-1,-1,-1, 0, 1,1,1 };

static int game_type;
static int moves;
static gboolean randomize;
static gboolean theme_default;
static char *theme;

static char *pref_theme;
static gboolean pref_theme_default;
static gboolean pref_randomize;
static int pref_game_type;

static struct game_values {
  int width;
  int height;
  gboolean cubeform;
  gboolean consistent;
  int states;
  /* From east to south east (like radians; 0 -> 2PI).  */
  gboolean neighbours[8];
} game;

enum game_types grid[MAX_WIDTH][MAX_HEIGHT];

static void new_game (void);

static void
draw_light (int x, int y)
{
  int bx, by;

  if (grid[x][y] != NO)
    {
      by = 0;
      bx = LIGHT_SIZE * (grid[x][y] - 1);

      gdk_draw_pixbuf (draw_area->window,
		       draw_area->style->black_gc, lights,
		       bx, by, x * LIGHT_SIZE, y * LIGHT_SIZE,
		       LIGHT_SIZE, LIGHT_SIZE, GDK_RGB_DITHER_NORMAL, 0, 0);
    }
  else
    gdk_window_clear_area (draw_area->window, x * LIGHT_SIZE, y * LIGHT_SIZE,
			   LIGHT_SIZE, LIGHT_SIZE);
}

static void
paint (GdkRectangle *area)
{
  int x1, y1, x2, y2, x, y;

  x1 = area->x / LIGHT_SIZE;
  y1 = area->y / LIGHT_SIZE;
  x2 = (area->x + area->width) / LIGHT_SIZE;
  y2 = (area->y + area->height) / LIGHT_SIZE;

  for (x = x1; x <= x2; x++){
    for (y = y1; y <= y2; y++){
      draw_light (x, y);
    }
  }
}

static void
game_over (void)
{
  gchar *msg;
  int r;
 
  msg = g_strdup_printf (_("Congratulations!\n\nYou made it with %d moves.\n\nPlay again?"), moves);

  r = gpe_question_ask (msg, _("Game over"), "icon", "!gtk-yes", NULL, "!gtk-no", NULL, NULL, NULL);

  g_free (msg);

  if (r == 0)
    new_game ();
}

static int
count_lights_on (void)
{
  int x, y;
  int counter = 0;

  for (x = 0; x < (game.cubeform ? 4 : 1) * game.width; x++)
    for (y = 0; y < (game.cubeform ? 3 : 1) * game.height; y++)
      if (grid[x][y] == ON || grid[x][y] == HALF)
	counter++;

  return counter;
}

static void
change_light (int x, int y)
{
  if (grid[x][y] == ON && game.states == 2)
    grid[x][y] = OFF;
  else if (grid[x][y] == ON && game.states == 3)
    grid[x][y] = HALF;
  else if (grid[x][y] == HALF)
    grid[x][y] = OFF;
  else if (grid[x][y] == OFF)
    grid[x][y] = ON;

  draw_light (x, y);
}

static void
set_lights_off (void)
{
  char buf [10];
  int nlo;

  nlo = game.width * game.height * (game.cubeform ? 6 : 1) - count_lights_on ();

  sprintf (buf, "%.2d", nlo);
  gtk_label_set (GTK_LABEL(lights_off), buf);
}

static void
set_moves (int new_moves)
{
  char buf [10];

  moves = new_moves;
  sprintf (buf, "%.2d ", moves);
  gtk_label_set (GTK_LABEL(moves_label), buf);
}

static void
select_light (int x, int y)
{
  int i;

  if (grid[x][y] == NO)
    return;

  change_light (x, y);

  if (game_type == MERLIN)
    {
      if (x != 0)
	change_light (x-1, y);
      if (x != 2)
	change_light (x+1, y);
      if (y != 0)
	change_light (x, y-1);
      if (y != 2)
	change_light (x, y+1);
      if (!(x == 1 && y == 1))
	change_light (1, 1);	
    }
  else
    {
      for (i = 0; i < 8; i++)
	if (game.neighbours[i])
	  {
	    if (game.cubeform)
	      {
		int tmp_x, tmp_y;

		tmp_x = x + x_dir[i];		  
		tmp_y = y + y_dir[i];		  

		if (tmp_y < 0)
		  {
		    tmp_y = game.height;
		    tmp_x = 5*game.width - tmp_x - 1;
		  }
		else if (y < game.height && tmp_x < game.width)
		  {
		    tmp_x = tmp_y;
		    tmp_y = game.height;
		  }
		else if (y < game.height && tmp_x >= 2*game.width)
		  {
		    tmp_x += game.width - tmp_y - 1;
		    tmp_y = game.height;
		  }
		else if (tmp_y < game.height && tmp_x < game.width)
		  {
		    tmp_y = tmp_x;
		    tmp_x = game.width;
		  }
		else if (tmp_y < game.height && tmp_x >= 3*game.width)
		  {
		    tmp_y = 0;
		    tmp_x = 5*game.width - tmp_x - 1;
		  }
		else if (tmp_y < game.height && tmp_x >= 2*game.width)
		  {
		    tmp_y = 3*game.width - tmp_x - 1;
		    tmp_x = 2*game.width - 1;
		  }
		else if (tmp_x < 0)
		  tmp_x = 4*game.width - 1;
		else if (tmp_x >= 4*game.width)
		  tmp_x = 0;
		else if (y >= 2*game.height && tmp_x < game.width)
		  {
		    tmp_x = 3*game.height - tmp_y - 1;
		    tmp_y = 2*game.height - 1;
		  }
		else if (y >= 2*game.height && tmp_x >= 2*game.width)
		  {
		    tmp_x = tmp_y;
		    tmp_y = 2*game.height - 1;
		  }
		else if (tmp_y >= 2*game.height && tmp_x < game.width)
		  {
		    tmp_y = 3*game.height - tmp_x - 1;
		    tmp_x = game.width;
		  }
		else if (tmp_y >= 2*game.height && tmp_x >= 3*game.width)
		  {
		    tmp_y = 3*game.height - 1;
		    tmp_x = 5*game.width - tmp_x - 1;
		  }
		else if (tmp_y >= 2*game.height && tmp_x >= 2*game.width)
		  {
		    tmp_y = tmp_x;
		    tmp_x = 2*game.width - 1;
		  }
		else if (tmp_y >= 3*game.height)
		  tmp_y = 0;

		change_light (tmp_x, tmp_y);
	      }
	    else if (game.consistent)
	      {
		int tmp_x, tmp_y;

		if ((x_dir[i] + x) < 0)
		  tmp_x = x + x_dir[i] + game.width;
		else if ((x_dir[i] + x) >= game.width)
		  tmp_x = x + x_dir[i] - game.width;
		else
		  tmp_x = x + x_dir[i];

		if ((y_dir[i] + y) < 0)
		  tmp_y = y + y_dir[i] + game.height;
		else if ((y_dir[i] + y) >= game.height)
		  tmp_y = y + y_dir[i] - game.height;
		else
		  tmp_y = y + y_dir[i];

		change_light (tmp_x, tmp_y);
	      }
	    else if ((x_dir[i] + x) >= 0 && (x_dir[i] + x) < game.width &&
		     (y_dir[i] + y) >= 0 && (y_dir[i] + y) < game.height)
	      change_light (x_dir[i] + x, y_dir[i] + y);
	  }
    }
}

static void
click_light (int x, int y)
{
  if (!count_lights_on ())
    return;

  select_light (x, y);

  if (grid[x][y] != NO)
    set_moves (++moves);
  set_lights_off ();

  gtk_widget_draw (draw_area, NULL);
  if (count_lights_on () == 0)
    game_over ();
}

static gint
area_event (GtkWidget *widget, GdkEvent *event, void *d)
{
  switch (event->type){
  case GDK_EXPOSE: {
    GdkEventExpose *e = (GdkEventExpose *) event;
    paint (&e->area);
    return TRUE;
  }
  case GDK_BUTTON_PRESS: {
    int x, y;
    gtk_widget_get_pointer (widget, &x, &y);
    click_light (x / LIGHT_SIZE, y / LIGHT_SIZE);
  }
  default:
    return FALSE;
  }
}

static void
game_randomize (void)
{
  int x, y, i;

  for (x = 0; x < (game.cubeform ? 4 : 1) * game.width; x++)
    for (y = 0; y < (game.cubeform ? 3 : 1) * game.height; y++)
      for (i = 0; i < rand () % game.states; i++)
	select_light (x, y);

  if (!count_lights_on ()) /* randomize() has "found" solution.  */
    {
      int i;

      for (i = 0; i <= game.states; i++)
	{
	  x = rand () % game.width;
	  y = rand () % game.height;

	  if (game.cubeform)
	    {
	      x += game.width * (rand () % 4);
	      if (x >= game.width && x < 2 * game.width)
		y += game.height * (rand () % 3);
	      else
		y += game.height;
	    }

	  select_light (x, y);
	}
    }

  gtk_widget_draw (draw_area, NULL);
  set_moves (0);
}

static void
reset_grid (void)
{
  int x, y;

  for (x = 0; x < (game.cubeform ? 4 : 1) * game.width; x++)
    for (y = 0; y < (game.cubeform ? 3 : 1) * game.height; y++)
      {
	grid[x][y] = ON;

	if (game.cubeform)
	  {
	    if (x < game.width)
	      if (y < game.height || y >= (2 * game.height))
		grid[x][y] = NO;
	    if (x >= 2 * game.width)
	      if (y < game.height || y >= (2 * game.height))
		grid[x][y] = NO;
	  }
      }
  if (randomize)
    game_randomize ();
}

static void
load_game_values (enum game_types type)
{
  int i;

  if (type == MERLIN)
    {
      game.width = 3;
      game.height = 3;
      game.cubeform = FALSE; /* not used */
      game.consistent = FALSE; /* not used */
      game.states = 2;
      /* The rules for neighbours in merlin is so different from the others
	 that it needs special treatment.  */
    }
  else if (type == KEYCHAIN)
    {
      game.width = 4;
      game.height = 4;
      game.cubeform = FALSE;
      game.consistent = TRUE;
      game.states = 2;
      for (i = 0; i < 8; i++)
	{
	  if ((i/2)*2 == i) /* i is even */
	    game.neighbours[i] = TRUE;
	  else /* i is odd */
	    game.neighbours[i] = FALSE;
	}
    }
  else if (type == CLASSIC)
    {
      game.width = 5;
      game.height = 5;
      game.cubeform = FALSE;
      game.consistent = FALSE;
      game.states = 2;
      for (i = 0; i < 8; i++)
	{
	  if ((i/2)*2 == i) /* i is even */
	    game.neighbours[i] = TRUE;
	  else /* i is odd */
	    game.neighbours[i] = FALSE;
	}
    }
  else if (type == TWOK)
    {
      game.width = 5;
      game.height = 5;
      game.cubeform = FALSE;
      game.consistent = FALSE;
      game.states = 3;
      for (i = 0; i < 8; i++)
	{
	  if ((i/2)*2 == i) /* i is even */
	    game.neighbours[i] = TRUE;
	  else /* i is odd */
	    game.neighbours[i] = FALSE;
	}
    }
  else if (type == DELUX)
    {
      game.width = 6;
      game.height = 6;
      game.cubeform = FALSE;
      game.consistent = FALSE;
      game.states = 2;
      for (i = 0; i < 8; i++)
	{
	  if ((i/2)*2 == i) /* i is even */
	    game.neighbours[i] = FALSE;
	  else /* i is odd */
	    game.neighbours[i] = TRUE;
	}
    }
  else if (type == CUBE)
    {
      game.width = 3;
      game.height = 3;
      game.cubeform = TRUE;
      game.consistent = TRUE; /* not used */
      game.states = 2;
      for (i = 0; i < 8; i++)
	{
	  if ((i/2)*2 == i) /* i is even */
	    game.neighbours[i] = TRUE;
	  else /* i is odd */
	    game.neighbours[i] = FALSE;
	}
    }
}

static void
new_game (void)
{
  char buf[10];
  int nl;

  reset_grid ();

  nl = game.width * game.height * (game.cubeform ? 6 : 1 );
  sprintf (buf, "/%.2d", nl);
  gtk_label_set (GTK_LABEL(lights_label), buf);

  set_moves (0);
  set_lights_off ();
  gtk_widget_draw (draw_area, NULL);
}

static void
load_theme (char *fname)
{
  char *fn;

  fn = g_strconcat (PREFIX "/share/" PACKAGE "/", fname, NULL);

  if (access (fn, F_OK))
    {
      printf (_("Could not find the \'%s\' theme for Lights Out\n"), fn);
      exit (1);
    }

  if (theme)
    g_free (theme);

  theme = g_strdup (fname);

  lights = gdk_pixbuf_new_from_file (fn, NULL);

  g_free (fn);

  gtk_widget_draw (draw_area, NULL);
}

static void
create_light_grid (char *fname)
{
  draw_area = gtk_drawing_area_new ();

  gtk_widget_pop_colormap ();
  gtk_widget_pop_visual ();

  gtk_widget_set_events (draw_area, gtk_widget_get_events (draw_area) | GAME_EVENTS);

  gtk_container_add (GTK_CONTAINER (align), draw_area);

  gtk_widget_realize (draw_area);
  gtk_widget_show (draw_area);

  load_theme (fname);

  gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
			 (game.cubeform ? 4 : 1) * game.width * LIGHT_SIZE,
			 (game.cubeform ? 3 : 1) * game.height * LIGHT_SIZE);
  gtk_signal_connect (GTK_OBJECT(draw_area), "event", (GtkSignalFunc) area_event, 0);
}

static void
pref_game_type_cb (GtkWidget *widget, int new_type)
{
  if (new_type == pref_game_type)
    return;

  if (!pref_dialog)
    return;

  pref_game_type = new_type;
}

static void
set_selection (GtkWidget *widget, char *new_theme)
{
  pref_theme = new_theme;
}

static void
pref_randomize_cb (GtkWidget *widget, void *data)
{
  pref_randomize = GTK_TOGGLE_BUTTON(widget)->active;
}

static void
free_cb (GtkWidget *widget, void *data)
{
  free (data);
}

static void
create_menu (GtkWidget *menu)
{
  struct dirent *e;
  char *dname = PREFIX "/share/" PACKAGE;
  DIR *dir;
  int itemno = 0;

  dir = opendir (dname);

  if (!dir)
    return;

  while ((e = readdir (dir)) != NULL)
    {
      GtkWidget *item;
      char *s = strdup (e->d_name);

      if (!strstr (e->d_name, ".png"))
	{
	  free (s);
	  continue;
	}

      item = gtk_menu_item_new_with_label (s);
      gtk_widget_show (item);
      gtk_menu_append (GTK_MENU(menu), item);
      gtk_signal_connect (GTK_OBJECT(item), "activate", (GtkSignalFunc)set_selection, s);
      gtk_signal_connect (GTK_OBJECT(item), "destroy", (GtkSignalFunc)free_cb, s);

      if (!strcmp (theme, s))
	gtk_menu_set_active (GTK_MENU(menu), itemno);

      itemno++;
    }
  closedir (dir);
}

static void
pref_cancel (GtkWidget *widget, void *data)
{
  gtk_widget_destroy (pref_dialog);
  pref_dialog = 0;
}

static void
pref_apply (GtkWidget *widget, void *data)
{
  if (pref_theme_default != theme_default)
    {
      theme_default = pref_theme_default;
#if 0
      gnome_config_set_bool ("/"PACKAGE"/Preferences/Themedef", theme_default);
#endif
    }

  if (pref_theme)
    load_theme (pref_theme);

#if 0
  if (theme_default)
    gnome_config_set_string ("/"PACKAGE"/Preferences/Theme", theme);
#endif

  if (pref_game_type != game_type)
    {
      game_type = pref_game_type;
#if 0
      gnome_config_set_int ("/"PACKAGE"/Preferences/Gametype", game_type);
#endif

      load_game_values (game_type);

      gtk_drawing_area_size (GTK_DRAWING_AREA (draw_area),
			     (game.cubeform ? 4 : 1) * game.width * LIGHT_SIZE,
			     (game.cubeform ? 3 : 1) * game.height * LIGHT_SIZE);

      new_game ();
    }

  if (pref_randomize != randomize)
    {
      randomize = pref_randomize;
#if 0
      gnome_config_set_bool ("/"PACKAGE"/Preferences/Randomize", randomize);
#endif
    }

  pref_cancel (0,0);
}

static void
prefs (void)
{
  GtkWidget *hbox, *cb, *frame, *fv, *vbox, *table, *button;
  GtkWidget *menu, *omenu;
  GtkWidget *ok, *cancel;
  guint width, height;
  gboolean portrait;
  
  if (pref_dialog)
    return;

  width = gdk_screen_width ();
  height = gdk_screen_height ();

  portrait = (height > width) ? TRUE : FALSE;

  pref_game_type = 0;
  pref_theme = 0;
  pref_theme_default = theme_default;

  pref_dialog = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (pref_dialog), _("Lights Out: Preferences"));
  gpe_set_window_icon (pref_dialog, "icon");

  gtk_widget_realize (pref_dialog);

  ok = gtk_button_new_from_stock (GTK_STOCK_OK);
  cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);

  gtk_signal_connect (GTK_OBJECT(pref_dialog), "delete_event", (GtkSignalFunc)pref_cancel, NULL);

  if (portrait)
    table = gtk_table_new (1, 5, FALSE);
  else
    table = gtk_table_new (2, 3, FALSE);
  gtk_widget_show (table);

  omenu = gtk_option_menu_new ();
  menu = gtk_menu_new ();
  create_menu (menu);
  gtk_widget_show (omenu);
  gtk_option_menu_set_menu (GTK_OPTION_MENU(omenu), menu);

  frame = gtk_frame_new (_("Theme"));
  gtk_container_border_width (GTK_CONTAINER (frame), 5);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);

  if (! portrait)
    {
      GtkWidget *label;

      label = gtk_label_new (_("Select theme: "));
      gtk_widget_show (label);

      gtk_box_pack_start_defaults (GTK_BOX(hbox), label);
    }

  gtk_box_pack_start_defaults (GTK_BOX(hbox), omenu);

  fv = gtk_vbox_new (0, 5);
  gtk_widget_show (fv);

  gtk_box_pack_start_defaults (GTK_BOX(fv), hbox);

  gtk_widget_show (frame);
  
  if (portrait)
    {
      gtk_table_attach (GTK_TABLE (table), fv, 0, 1, 3, 4, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
    }
  else
    {
      gtk_container_border_width (GTK_CONTAINER (fv), 5);
      gtk_container_add (GTK_CONTAINER (frame), fv);
      gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 0, 1, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
    }

  frame = gtk_frame_new (_("Game type"));
  gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
  gtk_widget_show (frame);
  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_widget_show (vbox);

  button = gtk_radio_button_new_with_label (NULL, "Merlin's Magic Square");
  if (game_type == MERLIN)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer) MERLIN);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Lights Out Keychain");
  if (game_type == KEYCHAIN)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer)KEYCHAIN);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Lights Out");
  if (game_type == CLASSIC)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer) CLASSIC);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Lights Out 2000");
  if (game_type == TWOK)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer) TWOK);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Lights Out Delux");
  if (game_type == DELUX)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer) DELUX);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Lights Out Cube");
  if (game_type == CUBE)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (pref_game_type_cb), (gpointer) CUBE);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_table_attach (GTK_TABLE (table), frame, 0, 1, 0, 3, GTK_EXPAND |
		    GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  frame = gtk_frame_new (_("Miscellaneous"));
  gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
  gtk_widget_show (frame);
  vbox = gtk_vbox_new (TRUE, 0);
  gtk_widget_show (vbox);

  cb = gtk_check_button_new_with_label (_("Randomize grid"));
  gtk_signal_connect (GTK_OBJECT(cb), "clicked", (GtkSignalFunc)pref_randomize_cb, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(cb), randomize);
  gtk_widget_show (cb);
  gtk_box_pack_start (GTK_BOX (vbox), cb, FALSE, FALSE, 0);

  if (portrait)
    {
      gtk_table_attach (GTK_TABLE (table), vbox, 0, 1, 4, 5, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
    }
  else
    {
      gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_table_attach (GTK_TABLE (table), frame, 1, 2, 1, 2, GTK_EXPAND |
			GTK_FILL, 0, 0, 0);
    }

  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), table);

  gtk_widget_show (ok);
  gtk_widget_show (cancel);
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->action_area), ok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->action_area), cancel, TRUE, TRUE, 0);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (pref_apply), NULL);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (pref_cancel), NULL);

  gtk_widget_show (pref_dialog);

  pref_game_type = game_type;
}

struct gpe_icon my_icons[] = {
  { "icon", PREFIX "/share/pixmaps/gpe-lights.png" },
  { NULL }
};

int
main (int argc, char *argv[])
{
  GtkWidget *label;
  GtkWidget *window;
  GtkWidget *toolbar;
  GtkWidget *pw;

  gpe_application_init (&argc, &argv);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);
  bind_textdomain_codeset (PACKAGE, "UTF-8");

  srand (time (NULL));

  gpe_load_icons (my_icons);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(gtk_main_quit),
		      NULL);

  vb = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vb);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);  

  gtk_box_pack_start (GTK_BOX (vb), toolbar, FALSE, FALSE, 0);

  gtk_widget_realize (window);
  gpe_set_window_icon (window, "icon");
  gtk_window_set_title (GTK_WINDOW (window), _("Lights Out"));

  pw = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("New"), _("New game"), _("Tap here to start a new game."),
			   pw, GTK_SIGNAL_FUNC (new_game), NULL);
  
  pw = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Prefs"), _("Preferences"), _("Tap here to configure Lights Out."),
			   pw, GTK_SIGNAL_FUNC (prefs), NULL);

  pw = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit the program."), pw, 
			   GTK_SIGNAL_FUNC (gtk_exit), NULL);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vb), align, TRUE, TRUE, 0);

  /* Set game values.  */
#if 0
  fname = gnome_config_get_string ("/"PACKAGE"/Preferences/Theme=bulb.png");
  game_type = gnome_config_get_int ("/"PACKAGE"/Preferences/Gametype=CLASSIC");
  sound = gnome_config_get_bool ("/"PACKAGE"/Preferences/Sound=TRUE");
  randomize = gnome_config_get_bool ("/"PACKAGE"/Preferences/Randomize=FALSE");
  theme_default = gnome_config_get_bool ("/"PACKAGE"/Preferences/Themedef=FALSE");
#endif

  fname = g_strdup ("bulb.png");

  load_game_values (game_type);
  create_light_grid (fname);
  g_free (fname);

  status_box = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_end (GTK_BOX(vb), status_box, FALSE, TRUE, 0);

  label = gtk_label_new (_("  Moves: "));
  moves_label = gtk_label_new ("");
  gtk_box_pack_end (GTK_BOX(status_box), moves_label, FALSE, TRUE, 0);
  gtk_box_pack_end (GTK_BOX(status_box), label, FALSE, TRUE, 0);

  label = gtk_label_new (_(" Lights off: "));
  lights_off = gtk_label_new ("00");
  lights_label = gtk_label_new ("/0");
  gtk_box_pack_start (GTK_BOX(status_box), label, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(status_box), lights_off, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(status_box), lights_label, FALSE, TRUE, 0);

  new_game ();

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
