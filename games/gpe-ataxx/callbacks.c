#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "gameutils.h"
#include "moteur.h"
#include "deepblue.h"

void sendCoords (Coords clickedCell);
void drawPlateau();

void
on_new_game1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
    Coords nullCoords;

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


void
on_quitter_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gtk_main_quit();
}

static Coords nullCoords = {0,0};

void
on_w_human_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jblordi = FALSE;
	sendCoords (nullCoords);
}


void
on_w_deep_blue_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jblordi = TRUE;
	sendCoords (nullCoords);
}


void
on_w_crazy_yellow_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jblordi = TRUE;
	sendCoords (nullCoords);
}


void
on_b_human_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jnoordi = FALSE;
	sendCoords (nullCoords);
}


void
on_b_deep_blue_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jnoordi = TRUE;
	sendCoords (nullCoords);
}


void
on_b_crazy_yellow_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	jnoordi = TRUE;
	sendCoords (nullCoords);
}


void
on_novice1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	depth = 1;
}


void
on_easy1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	depth = 3;
}


void
on_medium1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	depth = 4;
}


void
on_good1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	depth = 5;
}


void
on_crazy_yellows_props1_activate       (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

