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

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "moteur.h"
#include "gameutils.h"

#define ANIM_INTERVAL 60

GdkPixmap *cell_empty;
GdkPixmap *cell_black;
GdkPixmap *cell_white;

GdkBitmap *cell_empty_mask;
GdkBitmap *cell_black_mask;
GdkBitmap *cell_white_mask;

GdkPixmap *cells [13];
GdkBitmap *cells_mask [13];

static GtkWidget *pref_dialog;

GtkWidget *table [8][8];
int grilleAffichage [8][8];
short depth, old_depth;

GtkStatusbar *status;
guint status_cid;

GtkLabel *l_white,*l_black;

static int bouge = 0;
static Coords nullCoords = {0,0};

void smsg (const char *msg) {
	static guint mid = 0;
	if (mid)
		gtk_statusbar_pop (status,status_cid);
			
	mid = gtk_statusbar_push (status,status_cid,msg);
}

void drawPlateau ();
void initPlateau (GtkWidget *win);
gboolean on_cell_button_press_event        (GtkWidget       *widget,
																						GdkEventButton  *event,
																						gpointer         user_data);

int moteurResult = 0;

int gameIteration (gpointer inutile) {
//	printf ("gameIteration %d\n",moteurResult);
	if (!bouge) {
		if (moteurResult == 3) {
			/* Fin du jeu */
			smsg ("End of game");
		}
		else if (moteurResult != 0) {
			//		Coords nullCoords;
			//	char moteurResult;
			short playingColor;
			
			//		nullCoords.h = 0;
			//		nullCoords.v = 0;
			
			bouge = 1;
			playingColor = coulact;
			moteurResult = Moteur (nullCoords);
			if (abs(moteurResult) == 1) {
				if (moteurResult == blanc)
					smsg("White must skip a turn");
				if (moteurResult == noir)
					smsg("Black must skip a turn");
			}
			else if (moteurResult == 2) {
				char s[255];
				sprintf (s,"%s plays", coulact==noir?"Black":"White");
				smsg (s);
			}
			// FAIRE UNE PAUSE POUR GTK.. (genre 1 sec)
			//	while (bouge) {
			//		usleep (ANIM_INTERVAL*1000);
			//	}
			drawPlateau ();
		}
	}
	//	return moteurResult;
	//	printf ("gameIterationEND %d\n",moteurResult);
	return 1;
}

void sendCoords (Coords clickedCell)
{
	if (!bouge) {
		//    Coords nullCoords;
		//    char moteurResult;
    short playingColor;
		
		//    nullCoords.h = 0;
		//    nullCoords.v = 0;
		
		if ((canPlay(clickedCell, coulact)) || (clickedCell.h == 0)) {
			bouge = 1;
			playingColor = coulact;
			moteurResult = Moteur (clickedCell);
			if (moteurResult == 2) {
				char s[255];
				sprintf (s,"%s plays", coulact==noir?"Black":"White");
				smsg (s);
				drawPlateau ();
			}
			//			if (moteurResult != 3)
			//				do {
			//			gameIteration ();
			//					
			//				} while ((moteurResult !=0) && (moteurResult !=3));
			//			if (moteurResult == 3) {
			//				/* Fin du jeu */
			//				smsg ("End of game");
			// }
		}
		else {
			char s[255];
			sprintf (s,"Invalid move... %s plays", coulact==noir?"Black":"White");
			smsg (s);
		}
	}
}

void initPlateau (GtkWidget *win) {
	int h,v;
	GtkWidget *gtable;
	gtable = gtk_object_get_data (GTK_OBJECT(win),"game_table");
	
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "blackcell")),
									&cell_black,
									&cell_black_mask);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "whitecell")),
									&cell_white,
									&cell_white_mask);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "emptycell")),
									&cell_empty,
									&cell_empty_mask);
	
	cells [0] = cell_empty;
	cells_mask [0] = cell_empty_mask;
	cells [1] = cell_white;
	cells_mask [1] = cell_white_mask;
	cells [12] = cell_black;
	cells_mask [12] = cell_black_mask;

	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim01")),
									&cells[2],
									&cells_mask[2]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim02")),
									&cells[3],
									&cells_mask[3]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim03")),
									&cells[4],
									&cells_mask[4]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim04")),
									&cells[5],
									&cells_mask[5]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim05")),
									&cells[6],
									&cells_mask[6]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim06")),
									&cells[7],
									&cells_mask[7]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim07")),
									&cells[8],
									&cells_mask[8]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim08")),
									&cells[9],
									&cells_mask[9]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim09")),
									&cells[10],
									&cells_mask[10]);
	gtk_pixmap_get (GTK_PIXMAP (gtk_object_get_data (GTK_OBJECT(win),
																									 "anim10")),
									&cells[11],
									&cells_mask[11]);

	for (h=0;h<8;h++)
		for (v=0;v<8;v++) {
			GtkWidget *eventbox;
			Coords *coords;

			eventbox = gtk_event_box_new ();
			gtk_widget_show (eventbox);

			gtk_table_attach (GTK_TABLE(gtable),
												eventbox,
												h,h+1,v,v+1,
												(GtkAttachOptions) (GTK_FILL),
												(GtkAttachOptions) (GTK_FILL),
												0, 0);

			table [h][v] = gtk_pixmap_new (cell_empty,cell_empty_mask);
			gtk_widget_show (table [h][v]);
			gtk_container_add (GTK_CONTAINER (eventbox),table[h][v]);

			coords = (Coords*)malloc(sizeof(Coords));
			coords->h = h+1;
			coords->v = v+1;

			gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",
													GTK_SIGNAL_FUNC (on_cell_button_press_event),
													(void*)coords);
		}
}

int animatePlateau (gpointer inutile) {
	int h,v;
	int encore = 0;
	for (h=1;h<=8;h++)
		for (v=1;v<=8;v++) {
			int changed = 0;

			if (tableau[h][v] == noir) {
				changed = 1;
				if (grilleAffichage[h-1][v-1] == 0)
					grilleAffichage[h-1][v-1] = 12;
				else if (grilleAffichage[h-1][v-1] < 12)
					grilleAffichage[h-1][v-1] ++;
				else
					changed = 0;
			}

			if (tableau[h][v] == blanc) {
				changed = 1;
				if (grilleAffichage[h-1][v-1] == 0)
					grilleAffichage[h-1][v-1] = 1;
				else if (grilleAffichage[h-1][v-1] > 1)
					grilleAffichage[h-1][v-1] --;
				else
					changed = 0;
			}
			
			if ((tableau[h][v] == vide) && (grilleAffichage[h-1][v-1] != 0)) {
				changed = 1;
				grilleAffichage[h-1][v-1] = 0;
			}

			if (changed) {
				gtk_pixmap_set (GTK_PIXMAP(table[h-1][v-1]),
												cells[grilleAffichage[h-1][v-1]],cells_mask[grilleAffichage[h-1][v-1]]);
				encore ++;
			}
		}
	
	if (encore) {
		bouge = 1;
	}
	else {
		bouge = 0;
	}
	return 1;
}

void drawPlateau () {
	int h;
	char tmp [255];

	h = compter (noir);
	if (h > 1)
		sprintf (tmp, "%d cells", h);
	else if (h==0)
		strcpy (tmp, "No cells");
	else if (h==1)
		strcpy (tmp, "1 cell");
	gtk_label_set_text (l_black, tmp);

	h = compter (blanc);
	if (h > 1)
		sprintf (tmp, "%d cells", h);
	else if (h==0)
		strcpy (tmp, "No cells");
	else if (h==1)
		strcpy (tmp, "1 cell");
	gtk_label_set_text (l_white, tmp);
}

gboolean
on_cell_button_press_event        (GtkWidget       *widget,
																	 GdkEventButton  *event,
																	 gpointer         user_data) {
	Coords *coords = (Coords*)user_data;
	sendCoords (*coords);

	return FALSE;
}

static void
pref_cancel (GtkWidget *widget, void *data)
{
  depth=old_depth;
  /*if (depth == 1)
    on_novice1_activate();
  else if (depth == 2)
    on_easy1_activate();
  else if (depth == 4)
    on_medium1_activate();
  else on_good1_activate();*/
  gtk_widget_destroy (pref_dialog);
  pref_dialog = 0;
}

static void
pref_apply (GtkWidget *widget, void *data)
{
  Coords nullCoords;

  old_depth=depth;
  gtk_widget_destroy (pref_dialog);
  pref_dialog = 0;
  
  nullCoords.h = 0;
  nullCoords.v = 0;

  coulact = blanc;
  initTableau();
//    [theGameView syncGrids];
//    [theGameView setNeedsDisplay:TRUE];
//    [blackNumber setIntValue:compter(noir)];
//    [whiteNumber setIntValue:compter(blanc)];
		sendCoords(nullCoords);
		drawPlateau();
}

static void
prefs (void)
{
  GtkWidget *hbox, *cb, *frame, *fv, *vbox, *table, *button;
  GtkWidget *ok, *cancel;
  
  if (pref_dialog)
    return;

  pref_dialog = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (pref_dialog), _("Othello: Preferences"));
  gpe_set_window_icon (pref_dialog, "icon");

  gtk_widget_realize (pref_dialog);

  ok = gpe_picture_button (pref_dialog->style, _("OK"), "ok");
  cancel = gpe_picture_button (pref_dialog->style, _("Cancel"), "cancel");

  gtk_signal_connect (GTK_OBJECT(pref_dialog), "delete_event", (GtkSignalFunc)pref_cancel, NULL);

  frame = gtk_frame_new (_("Difficulty"));
  gtk_container_set_border_width (GTK_CONTAINER(frame), 5);
  gtk_widget_show (frame);
  vbox = gtk_vbox_new (TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER(vbox), 5);
  gtk_widget_show (vbox);

  old_depth=depth;
  
  button = gtk_radio_button_new_with_label (NULL, "Novice");
  if (depth == 1)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_novice1_activate), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Easy");
  if (depth == 2)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_easy1_activate), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Medium");
  if (depth == 4)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_medium1_activate), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label
    (gtk_radio_button_group (GTK_RADIO_BUTTON(button)), "Difficult");
  if (depth == 5)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (on_good1_activate), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), frame);

  gtk_widget_show (ok);
  gtk_widget_show (cancel);
  
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->action_area), ok, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->action_area), cancel, TRUE, TRUE, 0);
  
  gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (pref_apply), NULL);
  gtk_signal_connect (GTK_OBJECT (cancel), "clicked", GTK_SIGNAL_FUNC (pref_cancel), NULL);

  gtk_widget_show (pref_dialog);

}
struct gpe_icon my_icons[] = {
  { "new", },
  { "preferences" },
  { "icon", PREFIX "/share/pixmaps/gpe-othello.png" },
  { "exit", },
  { NULL }
};

int
main (int argc, char *argv[])
{
  int x,y;
  GtkWidget *window;
  GdkPixbuf *p;
  GtkWidget *pw;
  GtkWidget *vbox1;
  GtkWidget *align;
  GtkWidget *toolbar;
  GtkWidget *hbox1;
  GtkWidget *game_table;
  GtkWidget *eventbox_emptycell;
  GtkWidget *emptycell;
  GtkWidget *anim01;
  GtkWidget *anim02;
  GtkWidget *anim03;
  GtkWidget *anim04;
  GtkWidget *anim05;
  GtkWidget *anim06;
  GtkWidget *anim07;
  GtkWidget *anim08;
  GtkWidget *anim09;
  GtkWidget *anim10;
  GtkWidget *table2;
  GtkWidget *hseparator1;
  GtkWidget *hseparator2;
  GtkWidget *hseparator3;
  GtkWidget *whitecell;
  GtkWidget *blackcell;
  GtkWidget *label_white_point;
  GtkWidget *label_black_point;
  GtkWidget *statusbar1;
    
  gpe_application_init (&argc, &argv);

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  gpe_load_icons (my_icons);

  add_pixmap_directory (PREFIX "/share/gpe/pixmaps/default/" PACKAGE);
  add_pixmap_directory ("./pixmaps");

	jblordi = 1;
	jnoordi = 0;
	initOthello ();

	for (x=0;x<8;x++) for (y=0;y<8;y++) grilleAffichage [x][y] = 0;

  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(on_quitter_activate),
		      NULL);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), vbox1);

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);

  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);

  gtk_widget_realize (window);
  gpe_set_window_icon (window, "icon");
  gtk_window_set_title (GTK_WINDOW (window), _("Othello"));

  p = gpe_find_icon ("new");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("New"), _("New game"), _("Tap here to start a new game."),
			   pw, GTK_SIGNAL_FUNC (on_new_game1_activate), NULL);
  
  p = gpe_find_icon ("preferences");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar),
			   _("Prefs"), _("Preferences"), _("Tap here to configure Lights Out."),
			   pw, GTK_SIGNAL_FUNC (prefs), NULL);

  p = gpe_find_icon ("exit");
  pw = gtk_image_new_from_pixbuf(p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Exit"), 
			   _("Exit"), _("Exit the program."), pw, 
			   GTK_SIGNAL_FUNC (on_quitter_activate), NULL);

  align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox1), align, TRUE, TRUE, 0);
  
  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_set_name (hbox1, "hbox1");
  gtk_widget_ref (hbox1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox1", hbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  game_table = gtk_table_new (8, 8, TRUE);
  gtk_widget_set_name (game_table, "game_table");
  gtk_widget_ref (game_table);
  gtk_object_set_data_full (GTK_OBJECT (window), "game_table", game_table,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (game_table);
  gtk_box_pack_start (GTK_BOX (hbox1), game_table, FALSE, FALSE, 0);
  gtk_widget_set_usize (game_table, 170, 170);
  gtk_container_set_border_width (GTK_CONTAINER (game_table), 10);

  eventbox_emptycell = gtk_event_box_new ();
  gtk_widget_set_name (eventbox_emptycell, "eventbox_emptycell");
  gtk_widget_ref (eventbox_emptycell);
  gtk_object_set_data_full (GTK_OBJECT (window), "eventbox_emptycell", eventbox_emptycell,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (eventbox_emptycell);
  gtk_table_attach (GTK_TABLE (game_table), eventbox_emptycell, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  emptycell = create_pixmap (window, "EmptyCell.xpm");
  gtk_widget_set_name (emptycell, "emptycell");
  gtk_widget_ref (emptycell);
  gtk_object_set_data_full (GTK_OBJECT (window), "emptycell", emptycell,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_container_add (GTK_CONTAINER (eventbox_emptycell), emptycell);

  anim01 = create_pixmap (window, "01.xpm");
  gtk_widget_set_name (anim01, "anim01");
  gtk_widget_ref (anim01);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim01", anim01,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim01, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim02 = create_pixmap (window, "02.xpm");
  gtk_widget_set_name (anim02, "anim02");
  gtk_widget_ref (anim02);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim02", anim02,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim02, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim03 = create_pixmap (window, "03.xpm");
  gtk_widget_set_name (anim03, "anim03");
  gtk_widget_ref (anim03);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim03", anim03,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim03, 3, 4, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim04 = create_pixmap (window, "04.xpm");
  gtk_widget_set_name (anim04, "anim04");
  gtk_widget_ref (anim04);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim04", anim04,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim04, 4, 5, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim05 = create_pixmap (window, "05.xpm");
  gtk_widget_set_name (anim05, "anim05");
  gtk_widget_ref (anim05);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim05", anim05,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim05, 5, 6, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim06 = create_pixmap (window, "06.xpm");
  gtk_widget_set_name (anim06, "anim06");
  gtk_widget_ref (anim06);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim06", anim06,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim06, 6, 7, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim07 = create_pixmap (window, "07.xpm");
  gtk_widget_set_name (anim07, "anim07");
  gtk_widget_ref (anim07);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim07", anim07,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim07, 7, 8, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim08 = create_pixmap (window, "08.xpm");
  gtk_widget_set_name (anim08, "anim08");
  gtk_widget_ref (anim08);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim08", anim08,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim08, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);

  anim09 = create_pixmap (window, "09.xpm");
  gtk_widget_set_name (anim09, "anim09");
  gtk_widget_ref (anim09);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim09", anim09,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim09, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  anim10 = create_pixmap (window, "10.xpm");
  gtk_widget_set_name (anim10, "anim10");
  gtk_widget_ref (anim10);
  gtk_object_set_data_full (GTK_OBJECT (window), "anim10", anim10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_table_attach (GTK_TABLE (game_table), anim10, 2, 3, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  table2 = gtk_table_new (5, 2, FALSE);
  gtk_widget_set_name (table2, "table2");
  gtk_widget_ref (table2);
  gtk_object_set_data_full (GTK_OBJECT (window), "table2", table2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (table2);
  gtk_box_pack_start (GTK_BOX (hbox1), table2, TRUE, TRUE, 0);

  hseparator1 = gtk_hseparator_new ();
  gtk_widget_set_name (hseparator1, "hseparator1");
  gtk_widget_ref (hseparator1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator1", hseparator1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator1);
  gtk_table_attach (GTK_TABLE (table2), hseparator1, 0, 2, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  hseparator2 = gtk_hseparator_new ();
  gtk_widget_set_name (hseparator2, "hseparator2");
  gtk_widget_ref (hseparator2);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator2", hseparator2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator2);
  gtk_table_attach (GTK_TABLE (table2), hseparator2, 0, 2, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  hseparator3 = gtk_hseparator_new ();
  gtk_widget_set_name (hseparator3, "hseparator3");
  gtk_widget_ref (hseparator3);
  gtk_object_set_data_full (GTK_OBJECT (window), "hseparator3", hseparator3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hseparator3);
  gtk_table_attach (GTK_TABLE (table2), hseparator3, 0, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);

  whitecell = create_pixmap (window, "WhiteCell.xpm");
  gtk_widget_set_name (whitecell, "whitecell");
  gtk_widget_ref (whitecell);
  gtk_object_set_data_full (GTK_OBJECT (window), "whitecell", whitecell,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (whitecell);
  gtk_table_attach (GTK_TABLE (table2), whitecell, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 1, 0);
  gtk_misc_set_alignment (GTK_MISC (whitecell), 0, 0.5);

  blackcell = create_pixmap (window, "BlackCell.xpm");
  gtk_widget_set_name (blackcell, "blackcell");
  gtk_widget_ref (blackcell);
  gtk_object_set_data_full (GTK_OBJECT (window), "blackcell", blackcell,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (blackcell);
  gtk_table_attach (GTK_TABLE (table2), blackcell, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 1, 0);

  label_white_point = gtk_label_new ("No cells");
  gtk_widget_set_name (label_white_point, "label_white_point");
  gtk_widget_ref (label_white_point);
  gtk_object_set_data_full (GTK_OBJECT (window), "label_white_point", label_white_point,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_white_point);
  gtk_table_attach (GTK_TABLE (table2), label_white_point, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 1, 0);

  label_black_point = gtk_label_new ("No cells");
  gtk_widget_set_name (label_black_point, "label_black_point");
  gtk_widget_ref (label_black_point);
  gtk_object_set_data_full (GTK_OBJECT (window), "label_black_point", label_black_point,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label_black_point);
  gtk_table_attach (GTK_TABLE (table2), label_black_point, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);

  statusbar1 = gtk_statusbar_new ();
  gtk_widget_set_name (statusbar1, "statusbar1");
  gtk_widget_ref (statusbar1);
  gtk_object_set_data_full (GTK_OBJECT (window), "statusbar1", statusbar1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (statusbar1);
  gtk_box_pack_start (GTK_BOX (vbox1), statusbar1, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC (on_quitter_activate),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy_event",
                      GTK_SIGNAL_FUNC (on_quitter_activate),
                      NULL);
 
  gtk_widget_show_all(window);

  l_black = GTK_LABEL (gtk_object_get_data (GTK_OBJECT(window),"label_black_point"));
  l_white = GTK_LABEL (gtk_object_get_data (GTK_OBJECT(window),"label_white_point"));

  status = GTK_STATUSBAR (gtk_object_get_data (GTK_OBJECT(window),"statusbar1"));
  status_cid = gtk_statusbar_get_context_id (status,
																						 "Ptits messages");

  initPlateau (window);
  drawPlateau ();

  gtk_timeout_add (ANIM_INTERVAL,animatePlateau, NULL);
  gtk_timeout_add (ANIM_INTERVAL*5,gameIteration, NULL);

  gtk_main ();
  return 0;
}

