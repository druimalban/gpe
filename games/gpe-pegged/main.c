/*
 * main.c initial généré par Glade. Editer ce fichier à votre
 * convenance. Glade n'écrira plus dans ce fichier.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

//#include <pthread.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#include <libintl.h>

#define _(x)  gettext(x)

#define BOARD_SIZE 9
#define PIECE_SIZE 20

#define IMAGEDIR PREFIX "/share/gpe/pixmaps/default/gpe-pegged/"

GtkWidget *game_table=NULL;

GdkPixbuf *pixbuf_peg, *pixbuf_peg_sel, *pixbuf_hole;
GdkPixbuf *bg_outside, *bg_board;

int sel_x=-1, sel_y=-1;

int board[BOARD_SIZE][BOARD_SIZE];

int moves=0;

GtkWidget *status;
guint status_cid;

void smsg (const char *msg) {
        static guint mid = 0;
        if (mid)
                gtk_statusbar_pop (GTK_STATUSBAR(status),status_cid);
                                                                                
        mid = gtk_statusbar_push (GTK_STATUSBAR(status),status_cid,msg);
}

int is_board_position (int x, int y)
{
      if ( (x<3 && y<3) ||
           (x<3 && y>=6) ||
           (x>=6 && y<3) ||
           (x>=6 && y>=6) )
        return 0;
        return 1;
}

int count_pieces () {
	int x,y,count=0;
	for (x=0;x<BOARD_SIZE;x++)
		for (y=0;y<BOARD_SIZE;y++)
		{
			if (is_board_position (x,y) && board[x][y])
				count++;
		}
	return count;
}
void update_status ()
{
	char *msg;
	int pieces;

	pieces = count_pieces ();
	
	msg = g_strdup_printf ("%d moves; %d pieces left", moves, pieces);
	smsg (msg);
}

void init_board () {
  int x,y;
  for (x=0;x<BOARD_SIZE;x++)
    for (y=0;y<BOARD_SIZE;y++)
      board[x][y] = 1;

 board[4][4] = 0;
 moves=0;

 if (game_table)
   gtk_widget_queue_draw_area (game_table, 0, 0, game_table->allocation.width, game_table->allocation.height);

 if (status)
   smsg ("");
}

void draw_position (GtkWidget *widget, int x, int y, GdkPixbuf *pixbuf)
{
  int xoff;
  int yoff;

  xoff = 0;
  yoff = 0;

  gdk_draw_pixbuf (widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  pixbuf,
                  0,0,
                  x*PIECE_SIZE+xoff,y*PIECE_SIZE+yoff,
                  -1,-1,
                  GDK_RGB_DITHER_NORMAL,
                  0,0);
}

void draw_pieces (GtkWidget *widget)
{
  int x,y;
  for (x=0; x<BOARD_SIZE; x++)
	for (y=0; y<BOARD_SIZE; y++)
	{
		if (is_board_position(x,y))
		{
			if (x == sel_x && y == sel_y)
				draw_position (widget, x, y, pixbuf_peg_sel);
			else if (board[x][y])
				draw_position (widget, x, y, pixbuf_peg);
			else
				draw_position (widget, x, y, pixbuf_hole);
		}
	}
}

void
redraw_piece (GtkWidget *widget, int x, int y) {
   gtk_widget_queue_draw_area (widget, x*PIECE_SIZE, y*PIECE_SIZE, PIECE_SIZE, PIECE_SIZE);
}

gboolean
expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  int x,y;

  gdk_draw_pixbuf (widget->window,
		   widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
 		   bg_outside,
		   0,0,
		   0,0,
		   -1,-1,
		   GDK_RGB_DITHER_NORMAL,
		   0,0);

  // Draw the board BG
  for (x=0; x<BOARD_SIZE; x++) {
    for (y=0; y<BOARD_SIZE; y++) {
      if (is_board_position (x,y)) 
        draw_position (widget, x, y, bg_board);
    }
  }

  draw_pieces (widget);
  
  return TRUE;
}

gboolean
button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  int x,y;

  if (sel_x != -1)
    redraw_piece (widget, sel_x, sel_y);
  
  x = event->x / PIECE_SIZE;
  y = event->y / PIECE_SIZE;

  if (!is_board_position (x,y))
  {
    sel_x = -1, sel_y = -1;
    return TRUE;
  }
  
  if (board[x][y])
  { // Move the selection
    sel_x = x, sel_y = y;
    redraw_piece (widget, x,y);
  } else { // Move!
//	  printf ("Move %d/%d -> %d/%d (taking out %d/%d)\n",
//			  sel_x,sel_y,x,y,(sel_x+x)/2,(sel_y+y)/2);
	  if (y == sel_y && abs(sel_x - x) == 2 && board[(sel_x+x)/2][y] == 1)
	  {
		// OK
	  } else if (x == sel_x && abs(sel_y - y) == 2 && board[x][(sel_y+y)/2] == 1)
	  {
		// OK
	  } else
	  {
		// Can't move!
		  return TRUE;
	  }

          board[x][y] = 1;
          board[sel_x][sel_y] = 0;
          board[(sel_x+x)/2][(sel_y+y)/2] = 0;
	  redraw_piece (widget,x,y);
	  redraw_piece (widget,sel_x,sel_y);
	  redraw_piece (widget,(sel_x+x)/2, (sel_y+y)/2);

	  // Reset the selection
	  sel_x = sel_y = -1;

	  moves++;

	  update_status();
  }

  return TRUE;
}

void
on_new_game1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  init_board();
}

void
on_quitter_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
        gtk_main_quit();
}

struct gpe_icon my_icons[] = {
  { "new", },
  { "preferences" },
  { "icon", PREFIX "/share/pixmaps/gpe-pegged.png" },
  { "exit", },
  { NULL }
};

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkWidget *vbox1;
  GtkWidget *align;
  GtkWidget *toolbar;
    
  gpe_application_init (&argc, &argv);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  gpe_load_icons (my_icons);

//  add_pixmap_directory (PREFIX "/share/gpe/pixmaps/default/" PACKAGE);
//  add_pixmap_directory ("./pixmaps");

  // Load the pixbufs needed
  pixbuf_peg = gdk_pixbuf_new_from_file (IMAGEDIR"peg.xpm", NULL);
  pixbuf_hole = gdk_pixbuf_new_from_file (IMAGEDIR"hole.xpm", NULL);
  pixbuf_peg_sel = gdk_pixbuf_new_from_file (IMAGEDIR"peg_sel.xpm",NULL);
  bg_outside = gdk_pixbuf_new_from_file (IMAGEDIR"bg_out.png", NULL);
  bg_board = gdk_pixbuf_new_from_file (IMAGEDIR"bg_board.xpm", NULL);

  init_board ();

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox1);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);  

  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);

  gtk_widget_realize (window);
  gpe_set_window_icon (window, "icon");
  gtk_window_set_title (GTK_WINDOW (window), _("Pegged"));

  p = gpe_find_icon ("new");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("New"), _("New game"), _("Tap here to start a new game."),
			   pw, GTK_SIGNAL_FUNC (on_new_game1_activate), NULL);
/*  
  p = gpe_find_icon ("preferences");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Prefs"), _("Preferences"), _("Tap here to configure Ataxx"),
			   pw, GTK_SIGNAL_FUNC (prefs), NULL);
*/
  p = gpe_find_icon ("exit");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit the program."), pw, 
			   GTK_SIGNAL_FUNC (on_quitter_activate), NULL);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox1), align, TRUE, TRUE, 0);
  
  game_table = gtk_drawing_area_new ();
  gtk_widget_set_size_request (game_table, BOARD_SIZE*PIECE_SIZE,
					BOARD_SIZE*PIECE_SIZE);

  gtk_widget_set_name (game_table, "game_table");
  gtk_widget_ref (game_table);
  gtk_object_set_data_full (GTK_OBJECT (window), "game_table", game_table,
                            (GtkDestroyNotify) gtk_widget_unref);
  g_signal_connect (G_OBJECT (game_table), "expose_event",
				G_CALLBACK (expose_event_callback), NULL);
  g_signal_connect (G_OBJECT (game_table), "button-press-event",
                    G_CALLBACK (button_press), NULL);
  gtk_widget_add_events (game_table, GDK_BUTTON_PRESS_MASK);
  gtk_widget_show (game_table);
  gtk_box_pack_start (GTK_BOX (vbox1), game_table, TRUE, TRUE, 0);

  status = gtk_statusbar_new ();
  gtk_widget_set_name (status, "status");
  gtk_widget_ref (status);
  gtk_object_set_data_full (GTK_OBJECT (window), "status", status,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (status);
  gtk_box_pack_start (GTK_BOX (vbox1), status, FALSE, FALSE, 0);
  status_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR(status),
                 "Game status");


  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (on_quitter_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy_event",
                      GTK_SIGNAL_FUNC (on_quitter_activate),
                      NULL);
 
  gtk_widget_show_all(window);


  gtk_main ();
  return 0;
}
