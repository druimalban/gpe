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
#include <math.h>

#include <unistd.h>
#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#include <libintl.h>

#define _(x)  gettext(x)

#define BOARD_SIZE 9
#define PIECE_SIZE 20

#define ANIM_INTERVAL 30
#define TRAVEL 0.33

#define IMAGEDIR PREFIX "/share/gpe/pixmaps/default/gpe-pegged/"

GtkWidget *game_table=NULL;

GdkPixbuf *pixbuf_peg, *pixbuf_peg_sel, *pixbuf_hole;
GdkPixbuf *bg_outside, *bg_board;

int sel_x=-1, sel_y=-1;

int board[BOARD_SIZE][BOARD_SIZE];

int moves=0;

GtkWidget *status;
guint status_cid;

int is_animating=0;
int anim_x=0, anim_y=0;
float anim_at_x=-1,anim_at_y=-1;

void smsg (const char *msg) {
        static guint mid = 0;
        if (mid)
                gtk_statusbar_pop (GTK_STATUSBAR(status),status_cid);
                                                                                
        mid = gtk_statusbar_push (GTK_STATUSBAR(status),status_cid,msg);
}

int is_board_position (int x, int y)
{
      if ( // Corners
	   (x<3 && y<3) ||
           (x<3 && y>=6) ||
           (x>=6 && y<3) ||
           (x>=6 && y>=6) ||
	   // Boundaries
	   (x >= 9) || (y >= 9) ||
	   (x < 0) || (y < 0))
        return 0;
        return 1;
}

int can_move (int fx, int fy, int tx, int ty) {
	// Need to be on the board
	if (!is_board_position (fx, fy))
		return 0;
	if (!is_board_position (tx, ty))
		return 0;
	// Have to move a piece
	if (!board[fx][fy])
		return 0;
	// Have to have a spot to move it to
	if (board[tx][ty])
		return 0;
	
	// Can move horizontally?
	if (fy == ty && abs(fx - tx) == 2 && board[(fx+tx)/2][fy] == 1)
		return 1;
	// Can move vertically?
	if (fx == tx && abs(ty - fy) == 2 && board[fx][(fy+ty)/2] == 1)
		return 1;

	// Nup, can't move :-(
	return 0;
}

int has_moves () {
	int x,y;
	for (x=0;x<BOARD_SIZE;x++)
		for (y=0;y<BOARD_SIZE;y++)
		{
			// Move up?
			if (can_move(x,y,x,y-2))
				return 1;
			// Move down?
			if (can_move(x,y,x,y+2))
				return 1;
			// Move left?
			if (can_move(x,y,x-2,y))
				return 1;
			// Move right?
			if (can_move(x,y,x+2,y))
				return 1;
		}
	return 0;
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
	
	if (pieces <= 1)
		msg = g_strdup_printf ("You win!!!");
	else if (has_moves())
		msg = g_strdup_printf ("%d moves; %d pieces left", moves, pieces);
	else
		msg = g_strdup_printf ("Game Over!");
	smsg (msg);
}

void init_board () {
  int x,y;
  for (x=0;x<BOARD_SIZE;x++)
    for (y=0;y<BOARD_SIZE;y++)
      board[x][y] = 1;
//
//
 board[4][4] = 0;
 moves=0;
 sel_x = sel_y = -1;

 if (game_table)
   gtk_widget_queue_draw_area (game_table, 0, 0, game_table->allocation.width, game_table->allocation.height);

 if (status)
   smsg ("");
}

void draw_position (GtkWidget *widget, float x, float y, GdkPixbuf *pixbuf)
{
  int xoff;
  int yoff;

  xoff = 0;
  yoff = 0;

  /* Account for centering of the board */
  xoff += (game_table->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
  yoff += (game_table->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;

  /* Account for differing pixmap sizes */
  xoff += PIECE_SIZE/2 - gdk_pixbuf_get_width (pixbuf) / 2;
  yoff += PIECE_SIZE/2 - gdk_pixbuf_get_height (pixbuf) / 2;

  gdk_draw_pixbuf (widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  pixbuf,
                  0,0,
                  x*PIECE_SIZE+xoff,y*PIECE_SIZE+yoff,
                  -1,-1,
                  GDK_RGB_DITHER_NORMAL,
                  0,0);
}

void draw_pieces (GtkWidget *widget, GdkRectangle area)
{
  int x,y;
  int xoff, yoff;
  xoff = (widget->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
  yoff = (widget->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;

  for (x=0; x<BOARD_SIZE; x++)
	for (y=0; y<BOARD_SIZE; y++)
	{
		if (is_board_position(x,y))
		{
			GdkRectangle r,t;
			r.x = x*PIECE_SIZE+xoff;
			r.y = y*PIECE_SIZE+yoff;
			r.width = PIECE_SIZE;
			r.height = PIECE_SIZE;
			if (!gdk_rectangle_intersect (&area, &r, &t))
				continue;
			if (x == sel_x && y == sel_y && !is_animating)
				draw_position (widget, x, y, pixbuf_peg_sel);
			else if (board[x][y])
				draw_position (widget, x, y, pixbuf_peg);
			else
				draw_position (widget, x, y, pixbuf_hole);
		}
	}
}

int animBoard (gpointer data) {
	int xoff, yoff;
        if (!is_animating)
                return TRUE;
	xoff = (game_table->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
	yoff = (game_table->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;

	gtk_widget_queue_draw_area (GTK_WIDGET(data), MIN(anim_x*PIECE_SIZE,sel_x*PIECE_SIZE)-5+xoff,MIN(anim_y*PIECE_SIZE,sel_y*PIECE_SIZE)-5+yoff, abs(anim_x*PIECE_SIZE-sel_x*PIECE_SIZE)+PIECE_SIZE+10, abs(anim_y*PIECE_SIZE-sel_y*PIECE_SIZE)+PIECE_SIZE+10);


	return TRUE;
}

void
redraw_piece (GtkWidget *widget, int x, int y) {
	int xoff, yoff;
	xoff = (widget->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
	yoff = (widget->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
	gtk_widget_queue_draw_area (widget, x*PIECE_SIZE-5+xoff, y*PIECE_SIZE-5+yoff, PIECE_SIZE+10, PIECE_SIZE+10);
}

gboolean
expose_event_callback (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  int x,y;
  int xoff, yoff;
  xoff = (game_table->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
  yoff = (game_table->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;

  /* Main background */
 
  for (y=0;y<widget->allocation.height;y+=gdk_pixbuf_get_height(bg_outside))
    for (x=0;x<widget->allocation.width;x+=gdk_pixbuf_get_width (bg_outside)) {
      GdkRectangle r,r2;
      /* Pixmap rectangle */
      r.x = x; r.y = y;
      r.width = gdk_pixbuf_get_width (bg_outside);
      r.height = gdk_pixbuf_get_height (bg_outside);

      if (!gdk_rectangle_intersect (&r, &(event->area), &r2))
	      continue;

      gdk_draw_pixbuf (widget->window,
		       widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
 		       bg_outside,
		       r2.x - r.x, r2.y - r.y,
		       r2.x,r2.y,
		       r2.width, r2.height,
		       GDK_RGB_DITHER_NORMAL,
		       0,0);
    }
  
  // Draw the board BG
  for (x=0; x<BOARD_SIZE; x++) {
    for (y=0; y<BOARD_SIZE; y++) {
      if (is_board_position (x,y)) {
        GdkRectangle r,t;
        r.x = x*PIECE_SIZE+xoff;
        r.y = y*PIECE_SIZE+yoff;
        r.width = PIECE_SIZE;
        r.height = PIECE_SIZE;
	if (!gdk_rectangle_intersect (&r, &(event->area), &t))
		continue;
        draw_position (widget, x, y, bg_board);
      }
    }
  }

  draw_pieces (widget, event->area);
  
  if (sel_x != -1 && !is_animating) {
    draw_position (widget, sel_x, sel_y, pixbuf_peg_sel);
  }
  
  if (is_animating) {
	  if (anim_at_x == -1) { // First anim cycle
		  anim_at_x = sel_x;
		  anim_at_y = sel_y;
	  }
	  if (anim_at_x < anim_x)
		  anim_at_x += TRAVEL;
	  if (anim_at_x > anim_x)
		  anim_at_x -= TRAVEL;
	  if (anim_at_y < anim_y)
		  anim_at_y += TRAVEL;
	  if (anim_at_y > anim_y)
		  anim_at_y -= TRAVEL;


	  draw_position (widget, sel_x, sel_y, pixbuf_hole);

	  if (fabsf (anim_at_x - anim_x) < 1.0f &&
			  fabsf (anim_at_y - anim_y) < 1.0f)
		  draw_position (widget, (anim_x+sel_x)/2,
				  (anim_y+sel_y)/2, pixbuf_hole);
	  else
		  draw_position (widget, (anim_x+sel_x)/2,
				  (anim_y+sel_y)/2, pixbuf_peg);

            draw_position (widget, anim_at_x, anim_at_y, pixbuf_peg_sel);

	  if ( fabs (anim_at_x - (float)anim_x) < TRAVEL &&
			  fabs (anim_at_y - (float)anim_y) < TRAVEL)
		  {
	  is_animating = 0;
          board[anim_x][anim_y] = 1;
          board[sel_x][sel_y] = 0;
          board[(sel_x+anim_x)/2][(sel_y+anim_y)/2] = 0;
          redraw_piece (widget,anim_x,anim_y);
          redraw_piece (widget,sel_x,sel_y);
          redraw_piece (widget,(sel_x+anim_x)/2, (sel_y+anim_y)/2);
	  sel_x = anim_x;
	  sel_y = anim_y;
	  anim_at_x = anim_at_y = -1;
	  update_status();
		  }
  }

  return TRUE;
}

gboolean
button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  int x,y;
  int xoff, yoff;

  // For some backward reason, we get a btn1 & a btn2 event...
  if (event->button != 1)
	  return FALSE;
  
  if (sel_x != -1)
    redraw_piece (widget, sel_x, sel_y);
  
    /* Account for centering of the board */
  xoff = (widget->allocation.width/2) - ((BOARD_SIZE*PIECE_SIZE))/2;
  yoff = (widget->allocation.height/2) - ((BOARD_SIZE*PIECE_SIZE))/2;

  x = (event->x - xoff) / PIECE_SIZE;
  y = (event->y - yoff) / PIECE_SIZE;

  if (!is_board_position (x,y))
  {
    sel_x = -1, sel_y = -1;
    return FALSE;
  }
  
  if (x == sel_x && y == sel_y) {
    sel_x = sel_y = -1;	  
  } else if (board[x][y])
  { // Move the selection
    sel_x = x, sel_y = y;
    redraw_piece (widget, x,y);
  } else { // Move!
	  if (!can_move (sel_x, sel_y, x, y))
		  return FALSE;

	  moves++;

	  is_animating = 1;
	  anim_x = x, anim_y = y;

	  update_status();
  }

  return FALSE;
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

  gtk_timeout_add (ANIM_INTERVAL,animBoard, game_table);


  gtk_main ();
  return 0;
}
