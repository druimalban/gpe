#include "globals.h"
#include "gameutils.h"
#include "deepblue.h"
#include "moteur.h"

void initAtaxx()
{
	initTableau();    /* initialise le tableau du jeu */
	
	depth = 4;   /* profondeur de l'arborescence de l'IA par defaut */
	coulact = TRUE; /* couleur qui commence le jeu */
	jblordi = FALSE; /* true si l'ordinateur controle les blancs, false sinon */
	jnoordi = TRUE; /* true si l'ordinateur controle les noirs, false sinon */
	quit = FALSE;
}

char Moteur (Coords sel, Coords playedCoords)  /* moteur principal du jeu */
{
	char playerAction = (playedCoords.h != 0);
	char retour = 0;

	caseJouee.h = 0;
	caseJouee.v = 0;
	
	if (compter(blanc) == 0 || compter(noir) == 0) {
//		printf ("No pieces left for one player\n");
		return 3;
	}
	if (compter(blanc) + compter(noir) == 8*8) {
//		printf ("Game finished\n");
		return 3;
	}
	if (!moves_possible (coulact)) {
//		printf ("No possible moves for the colour\n");
		return 3;
	}
	copieTableau(&tableau, &etatPrecedent);  /* on sauvegarde l'etat precedent du tableau */
	if (coulact == blanc) { /* on fait jouer l'ordinateur ou l'utilisateur, suivant le cas rencontre */
			if (jblordi)
			IA(&sel, &caseJouee);
		else {
			if (playerAction)
				caseJouee = playedCoords;
		}
	}
	if (coulact == noir) {
		if (jnoordi)
			IA(&sel, &caseJouee);
		else {
			if (playerAction)
				caseJouee = playedCoords;
		}
	}

	if (caseJouee.h != 0) {
		Joue(sel, caseJouee, coulact);  /* on effectue le coup selectionne */
		/* Clignotte */
		retour = 2;
	}
		
	if (retour)
		coulact = -coulact;
	return retour;
}

static int moves_possible (coulact) {
	int x,y;
	for (y=1;y<=8;y++)
		for (x=1;x<=8;x++)
		{
			if (tableau[x][y] == coulact)
			{
				int x2,y2;
				for (y2=y-2;y2<=y+2;y2++) {
					for (x2=x-2;x2<=x+2;x2++) {
						Coords from, to;
						from.h = x; from.v = y;
						to.h = x2; to.v = y2;
						if (canPlay (from, to, coulact))
							return 1;
					}
				}
			}
		}
	return 0;
}
