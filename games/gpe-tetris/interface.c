#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <libintl.h>

#include "tetris.h"

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>

#define _(_x) gettext (_x)

#ifdef BIGBLOCKS
#include "big_blocks.xpm"
#else
#include "blocks.xpm"
#endif

GtkWidget *main_window;
GtkWidget *score_label;
GtkWidget *level_label;
GtkWidget *lines_label;
GtkWidget *menu_game_start;
GtkWidget *menu_game_stop;
GtkWidget *menu_game_pause;
GtkWidget *menu_game_show_next_block;
GtkWidget *spin_level;
GtkWidget *spin_noise_level;
GtkWidget *spin_noise_height;
GtkWidget *spin_noise_height;
GtkWidget *new_game_window;
GtkWidget *new_highscore_window;
GtkWidget *highscore_name_entry;
gint timer;

int level_speeds[NUM_LEVELS] = 
	{1000,886,785,695,616,546,483,428,379,336,298,264,234,207,183,162,144,127,113,100};	

struct gpe_icon my_icons[] = {
  { "new", "new" },
  { "help", "help" },
  { "exit", "exit" },
  { "pause", "tetris/pause" },
  { "stop", "tetris/stop" },
  { "highscores", "tetris/highscores" },
  { "icon", PREFIX "/share/pixmaps/gpe-tetris.png" },
  {NULL, NULL}
};

static void
close_window(GtkWidget *widget,
	     GtkWidget *w)
{
  gtk_widget_destroy (w);
}

void update_game_values()
{
	char dummy[20] = "";

	sprintf(dummy,"Score:\n%lu",current_score);
	set_label(score_label,dummy);
	sprintf(dummy,"Level:\n%d",current_level);
	set_label(level_label,dummy);
	sprintf(dummy,"Lines:\n%d",current_lines);
	set_label(lines_label,dummy);
}

gint keyboard_event_handler(GtkWidget *widget,GdkEventKey *event,gpointer data)
{
	int dropbonus = 0;
	
	if(game_over || game_pause)
		return TRUE;
	
	switch(event->keyval)
	{
		case GDK_x: case GDK_X: move_block(0,0,1); break;
		case GDK_w: case GDK_W: case GDK_Up: move_block(0,0,-1); break;
		case GDK_s: case GDK_S:
#ifndef DOWN_DROPS 
	        case GDK_Down: 
#endif
			move_down(); break;
		case GDK_a: case GDK_A: case GDK_Left: move_block(-1,0,0); break;
		case GDK_d: case GDK_D: case GDK_Right: move_block(1,0,0); break;
		case GDK_space:
#ifdef DOWN_DROPS
		case GDK_Down:
#endif
			while(move_down())
				dropbonus++;
			current_score += dropbonus*(current_level+1);
			update_game_values();
			break;
	}
	return TRUE;
}

gint game_area_expose_event(GtkWidget *widget, GdkEventExpose *event)
{	
	if(!game_over)
	{
		from_virtual();
		move_block(0,0,0); 
	}
	else
		gdk_draw_rectangle(widget->window,
			widget->style->black_gc,
			TRUE,
			0,0,
			widget->allocation.width,
			widget->allocation.height);
	return FALSE;
}

gint next_block_area_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
	gdk_draw_rectangle(widget->window,
		widget->style->black_gc,
		TRUE,
		0,0,
		widget->allocation.width,
		widget->allocation.height);
	if(!game_over && show_next_block)
		draw_block(0,0,next_block,next_frame,FALSE,TRUE);
	return FALSE;
}

int game_loop()
{
	if(!game_over)
	{
		timer = gtk_timeout_add(level_speeds[current_level],(GtkFunction)game_loop,NULL);
		move_down();
	}
	return FALSE;
}

void game_set_pause()
{
	if(game_over)
	{
  //		gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menu_game_pause),FALSE);
		return;
	}
	game_pause = !game_pause;
	if(game_pause)
		gtk_timeout_remove(timer);
	else
		timer = gtk_timeout_add(level_speeds[current_level],(GtkFunction)game_loop,NULL);
}

void add_new_highscore(GtkWidget *widget,gpointer *data)
{
	int high_dummy;
	read_highscore();
	if(current_score && (high_dummy = addto_highscore((char *)gtk_entry_get_text(GTK_ENTRY (highscore_name_entry)),current_score)))
	{
		write_highscore();
		show_highscore(high_dummy);
	}
	gtk_widget_destroy(new_highscore_window);
}

void new_highscore_name()
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *ok;
  GtkWidget *buttons = gtk_hbox_new (FALSE, 0);
  GtkWidget *label = gtk_label_new ("Your Name:");
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

  new_highscore_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  
  highscore_name_entry = gtk_entry_new ();
  ok = gtk_button_new_from_stock (GTK_STOCK_OK);

  gtk_widget_show (ok);
  gtk_widget_show (buttons);
  gtk_widget_show (label);
  gtk_widget_show (hbox);
  gtk_widget_show (highscore_name_entry);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), highscore_name_entry, TRUE, TRUE, 2);

  gtk_box_pack_end (GTK_BOX (buttons), ok, FALSE, FALSE, 2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (vbox), buttons, FALSE, FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked",
		      GTK_SIGNAL_FUNC (add_new_highscore), NULL);

  gtk_widget_show (vbox);

  gtk_window_set_title (GTK_WINDOW (new_highscore_window), _("Highscore"));

  gtk_container_add (GTK_CONTAINER (new_highscore_window), vbox);
  gtk_widget_show (new_highscore_window);
  gtk_widget_grab_focus (highscore_name_entry);
}

void game_over_init()
{
	new_highscore_name();

	game_over = TRUE;
	gdk_draw_rectangle(game_area->window,
		game_area->style->black_gc,
		TRUE,
		0,0,
		game_area->allocation.width,
		game_area->allocation.height);

	gdk_draw_rectangle(next_block_area->window,
		next_block_area->style->black_gc,
		TRUE,
		0,0,
		next_block_area->allocation.width,
		next_block_area->allocation.height);
	game_set_pause();
	gtk_widget_set_sensitive(menu_game_start,TRUE);
	gtk_widget_set_sensitive(menu_game_stop,FALSE);
	gtk_widget_set_sensitive(menu_game_pause,FALSE);
	gtk_timeout_remove(timer);
}

void game_start_stop(GtkWidget *widget,gpointer *data)
{
	gtk_widget_set_sensitive(widget, FALSE);
	if(data)
	{
		gtk_widget_set_sensitive(menu_game_stop,TRUE);
		gtk_widget_set_sensitive(menu_game_pause,TRUE);
		gtk_widget_set_sensitive(menu_game_start,FALSE);
		game_init();
		timer = gtk_timeout_add(level_speeds[current_level],(GtkFunction)game_loop,NULL);
	}
	else
	{
		game_over_init();
		gtk_widget_set_sensitive(menu_game_start,TRUE);
	}
}

void show_about()
{
	GtkWidget *about_window;
	GtkWidget *about_label;
	GtkWidget *about_border;
	GtkWidget *v_box;
	
	about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(about_window),_("About"));
	gtk_window_set_policy(GTK_WINDOW(about_window),FALSE,FALSE,TRUE);
	gtk_window_set_position(GTK_WINDOW(about_window),GTK_WIN_POS_CENTER);
	gtk_container_border_width(GTK_CONTAINER(about_window),1);
	
	about_border = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(about_border),GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(about_window),about_border);
	
	v_box = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(about_border),v_box);
		
	about_label = gtk_label_new(	"\nJust another GTK Tetris v0.5\n\n"
					"(c)1999,2000 Mattias Wadman\n"
					"napolium@sudac.org\n\n"
					"This program is distributed under the terms of GPL.\n");
	gtk_box_pack_start(GTK_BOX(v_box),about_label,FALSE,FALSE,0);
	
	gtk_widget_show_all(about_window);
}

void show_help()
{
	GtkWidget *help_window;
	GtkWidget *help_label;
	GtkWidget *help_border;
	GtkWidget *vbox;
	GtkWidget *button;
	
	help_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(help_window),"Help");
	gtk_window_set_policy(GTK_WINDOW(help_window),FALSE,FALSE,TRUE);
	gtk_window_set_position(GTK_WINDOW(help_window),GTK_WIN_POS_CENTER);
	gtk_container_border_width(GTK_CONTAINER(help_window),1);
	
	help_border = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(help_border),GTK_SHADOW_OUT);
	gtk_container_add(GTK_CONTAINER(help_window),help_border);

	vbox = gtk_vbox_new(FALSE,30);
	gtk_container_add(GTK_CONTAINER(help_border),vbox);

	help_label = gtk_label_new(	"Key:\n"
					"Right and \"a\"\n"
					"Left and \"d\"\n"
#ifndef DOWN_DROPS
					"Down and "
#endif
					"\"s\"\n"
					"Up and \"w\"\n"
					"\"x\"\n"
#ifdef DOWN_DROPS
					"Down and "
#endif
					"Space\n\n"
					"Score: score*level\n"
					"Single\n"
					"Double\n"
					"Triple\n"
					"TETRIS\n\n"
					"Drop bonus: rows*level\n");
	gtk_misc_set_alignment(GTK_MISC(help_label),0,0);	
	gtk_label_set_justify(GTK_LABEL(help_label),GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(vbox),help_label,TRUE,TRUE,TRUE);

	help_label = gtk_label_new(	"\n"
					"move right\n"
					"move left\n"
					"move down\n"
					"rotate ccw\n"
					"rotate cw\n"
					"drop block\n\n\n"
					"40\n100\n"
					"300\n1200\n");
	gtk_misc_set_alignment(GTK_MISC(help_label),0,0);	
	gtk_label_set_justify(GTK_LABEL(help_label),GTK_JUSTIFY_LEFT);
	gtk_box_pack_start(GTK_BOX(vbox),help_label,TRUE,TRUE,TRUE);
	
	button = gpe_picture_button (main_window->style, _("Close"), "cancel");
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
				GTK_SIGNAL_FUNC(close_window),help_window);	
	gtk_box_pack_start(GTK_BOX(vbox),button,FALSE,TRUE,0);
  	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	
	gtk_widget_show_all(help_window);
}

void game_new_wrapper()
{
	int new_level,new_noise_level,new_noise_height;

	new_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_level));
	new_noise_level = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_noise_level));
	new_noise_height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin_noise_height));
	
	gtk_widget_set_sensitive(menu_game_start,FALSE);
	gtk_widget_set_sensitive(menu_game_stop,TRUE);
	gtk_widget_set_sensitive(menu_game_pause,TRUE);
	
	game_init();
	make_noise(new_noise_level,new_noise_height);
	from_virtual();
	move_block(0,0,0);
	current_level = new_level;
	update_game_values(0,current_level,0);
	timer = gtk_timeout_add(level_speeds[current_level],(GtkFunction)game_loop,NULL);
	gtk_widget_hide(new_game_window);
}

void show_new_game_close(int close)
{
	gtk_widget_set_sensitive(menu_game_start,TRUE);
	if(close)
		gtk_widget_hide(new_game_window);
}


void show_new_game()
{
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *vbox,*hbox;
	GtkWidget *table;
	GtkWidget *button;
	GtkAdjustment *adj;
	
	new_game_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_signal_connect(GTK_OBJECT(new_game_window),"destroy",
				GTK_SIGNAL_FUNC(show_new_game_close),GINT_TO_POINTER(FALSE));
	
	gtk_window_set_title(GTK_WINDOW(new_game_window),"New game");
	gtk_window_set_position(GTK_WINDOW(new_game_window),GTK_WIN_POS_CENTER);
	gtk_container_border_width(GTK_CONTAINER(new_game_window),3);
	
	vbox = gtk_vbox_new(FALSE,2);
	gtk_container_add(GTK_CONTAINER(new_game_window),vbox);
	
	frame = gtk_frame_new("Game settings:");
	gtk_box_pack_start(GTK_BOX(vbox),frame,TRUE,TRUE,TRUE);

	table = gtk_table_new(3,2,TRUE);
	gtk_container_add(GTK_CONTAINER(frame),table);
	
	label = gtk_label_new("Start level:");
	adj = (GtkAdjustment *)gtk_adjustment_new(0,0,NUM_LEVELS-1,1,1,0);
	spin_level = gtk_spin_button_new(adj,0,0);	
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table),spin_level,1,2,0,1);
	
	label = gtk_label_new("Noise level:");
	adj = (GtkAdjustment *)gtk_adjustment_new(0,0,MAX_X-1,1,1,0);
	spin_noise_level = gtk_spin_button_new(adj,0,0);	
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,1,2);
	gtk_table_attach_defaults(GTK_TABLE(table),spin_noise_level,1,2,1,2);
	
	label = gtk_label_new("Noise height:");
	adj = (GtkAdjustment *)gtk_adjustment_new(0,0,MAX_Y-4,1,1,0);
	spin_noise_height = gtk_spin_button_new(adj,0,0);	
	gtk_table_attach_defaults(GTK_TABLE(table),label,0,1,2,3);
	gtk_table_attach_defaults(GTK_TABLE(table),spin_noise_height,1,2,2,3);
	
	hbox = gtk_hbox_new(TRUE,0);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,TRUE,0);

	gtk_widget_realize (new_game_window);
	button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
				GTK_SIGNAL_FUNC(show_new_game_close),GINT_TO_POINTER(TRUE));	
	gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,TRUE,0);
  	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	
	button = gpe_picture_button (new_game_window->style,
				     _("Start game"), "ok");
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
				GTK_SIGNAL_FUNC(game_new_wrapper),
				NULL);	
	gtk_box_pack_start(GTK_BOX(hbox),button,FALSE,TRUE,0);
  	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    	gtk_widget_grab_default (button);
		
	gtk_widget_set_usize(new_game_window,220,130);
	gtk_widget_show_all(new_game_window);
	gtk_widget_set_sensitive(menu_game_start,FALSE);
}

void show_highscore_wrapper()
{
	read_highscore();
	show_highscore(0);
}

void game_show_next_block()
{
	show_next_block = !show_next_block;
	if(!game_over) {
		if(!show_next_block)
			draw_block(0,0,next_block,next_frame,TRUE,TRUE);
		else
			draw_block(0,0,next_block,next_frame,FALSE,TRUE);
	}
}


int main(int argc,char *argv[])
{	
	GtkWidget *v_box;
	GtkWidget *h_box;
	GtkWidget *right_side;
	GtkWidget *game_border;	
	GtkWidget *next_block_border;
	GtkWidget *toolbar, *toolbar2, *toolbar_hbox;
	GdkBitmap *mask;
	GtkWidget *pw;
	GdkPixbuf *p;
	
	gtk_init(&argc,&argv);

	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);

	if (gpe_load_icons (my_icons) == FALSE)
		exit (1);


	setlocale (LC_ALL, "");
	
	bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
	textdomain (PACKAGE);	

	// init game values
	game_over = TRUE;
	game_pause = FALSE;
	current_x = current_y = 0;
	current_block = current_frame = 0;
	current_score = current_level = current_lines = 0;
	next_block = next_frame = 0;
	show_next_block = TRUE;
	
	// window
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_policy(GTK_WINDOW(main_window),FALSE,FALSE,TRUE);
	gtk_signal_connect(GTK_OBJECT(main_window),"delete_event",
			GTK_SIGNAL_FUNC(gtk_main_quit),NULL);
	gtk_window_set_title(GTK_WINDOW(main_window),"GPE Tetris");
	gtk_signal_connect(GTK_OBJECT(main_window),"key_press_event",
			GTK_SIGNAL_FUNC(keyboard_event_handler),NULL);
	
	// vertical box
	v_box = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(main_window),v_box);
	gtk_widget_show(v_box);
	
	// toolbar horizontal box
	toolbar_hbox = gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(v_box),toolbar_hbox,FALSE,FALSE,0);
	gtk_widget_show(toolbar_hbox);

	// toolbar
	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start(GTK_BOX(toolbar_hbox),toolbar,FALSE,FALSE,0);
	gtk_widget_show(toolbar);

	pw = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_SMALL_TOOLBAR);
	menu_game_start = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New game"), 
			   _("New game"), _("New game"), pw, show_new_game, NULL);

	p = gpe_find_icon ("stop");
  pw = gtk_image_new_from_pixbuf (p);
	menu_game_stop = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop game"), 
			   _("Stop game"), _("Stop game"), pw,  GTK_SIGNAL_FUNC(game_start_stop), NULL);
	gtk_widget_set_sensitive(menu_game_stop,FALSE);

	p = gpe_find_icon ("pause");
  pw = gtk_image_new_from_pixbuf (p);
	menu_game_pause = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Pause game"), 
			   _("Pause game"), _("Pause game"), pw, game_set_pause, NULL);
	gtk_widget_set_sensitive(menu_game_pause,FALSE);

	p = gpe_find_icon ("highscores");
  pw = gtk_image_new_from_pixbuf (p);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Highscores"), 
			   _("Highscores"), _("Highscores"), pw, show_highscore_wrapper, NULL);

	// toolbar 2
	toolbar2 = gtk_toolbar_new ();
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar2), GTK_TOOLBAR_ICONS);
        gtk_box_pack_end(GTK_BOX(toolbar_hbox),toolbar2,FALSE,FALSE,0);
	gtk_widget_show(toolbar2);

	pw = gtk_image_new_from_stock (GTK_STOCK_HELP, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Help"), 
			   _("Help"), _("Help"), pw, show_help, NULL);

	pw = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Exit"), 
			   _("Exit"), _("Exit"), pw, gtk_main_quit, NULL);

	// horizontal box
	h_box = gtk_hbox_new(FALSE,1);
	gtk_widget_show(h_box);
	gtk_box_pack_start(GTK_BOX(v_box),h_box,FALSE,FALSE,0);
	
	// game_border
	game_border = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(game_border),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(h_box),game_border,FALSE,FALSE,1);
	gtk_widget_show(game_border);

	// game_area
	game_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(game_area),MAX_X*BLOCK_WIDTH,MAX_Y*BLOCK_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(game_area), "expose_event",
			(GtkSignalFunc) game_area_expose_event, NULL);
	
	gtk_widget_set_events(game_area, GDK_EXPOSURE_MASK);
	gtk_container_add(GTK_CONTAINER(game_border),game_area);
	gtk_widget_show(game_area);

	// right_side
	right_side = gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(h_box),right_side,FALSE,FALSE,0);
	gtk_widget_show(right_side);
	
	// next_block_border
	next_block_border = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(next_block_border),GTK_SHADOW_IN);
	gtk_box_pack_start(GTK_BOX(right_side),next_block_border,FALSE,FALSE,0);
	gtk_widget_show(next_block_border);
	
	// next_block_area
	next_block_area = gtk_drawing_area_new();
	gtk_drawing_area_size(GTK_DRAWING_AREA(next_block_area),4*BLOCK_WIDTH,4*BLOCK_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(next_block_area), "expose_event",
			(GtkSignalFunc) next_block_area_expose_event, NULL);
	
	gtk_widget_set_events(next_block_area, GDK_EXPOSURE_MASK);
	gtk_container_add(GTK_CONTAINER(next_block_border),next_block_area);
	gtk_widget_show(next_block_area);
	
	// the score,level and lines labels
	score_label = gtk_label_new("Score:\n0");
	gtk_label_set_justify(GTK_LABEL(score_label),GTK_JUSTIFY_RIGHT);
	gtk_widget_show(score_label);
	gtk_box_pack_start(GTK_BOX(right_side),score_label,FALSE,FALSE,3);

	level_label = gtk_label_new("Level:\n0");
	gtk_label_set_justify(GTK_LABEL(level_label),GTK_JUSTIFY_RIGHT);
	gtk_widget_show(level_label);
	gtk_box_pack_start(GTK_BOX(right_side),level_label,FALSE,FALSE,3);

	lines_label = gtk_label_new("Lines:\n0");
	gtk_label_set_justify(GTK_LABEL(lines_label),GTK_JUSTIFY_RIGHT);
	gtk_widget_show(lines_label);
	gtk_box_pack_start(GTK_BOX(right_side),lines_label,FALSE,FALSE,3);

	gpe_set_window_icon (main_window, "icon");
	
	// show window widget
	gtk_widget_show(main_window);

	// Block images...
	blocks_pixmap = gdk_pixmap_create_from_xpm_d(game_area->window,
						&mask,
						NULL,
						(gchar **)blocks_xpm); 
	// seed random generator
	srandom(time(NULL));

	// gtk_main
	gtk_main();

	return 0; 
}
