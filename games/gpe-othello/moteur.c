#include "globals.h"
#include "gameutils.h"
#include "deepblue.h"
#include "moteur.h"

void initOthello()
{
	initTableau();    /* initialise le tableau du jeu */
	
	depth = 4;   /* profondeur de l'arborescence de l'IA par defaut */
	coulact = TRUE; /* couleur qui commence le jeu */
	jblordi = FALSE; /* true si l'ordinateur controle les blancs, false sinon */
	jnoordi = TRUE; /* true si l'ordinateur controle les noirs, false sinon */
	quit = FALSE;
}

char Moteur (Coords playedCoords)  /* moteur principal du jeu */
{
	
	char playerAction = (playedCoords.h != 0);
	char retour = 0;
	
	caseJouee.h = 0;
	caseJouee.v = 0;
	
	if (passe(coulact)) {
		if (passe(-coulact)) {
			/*si le joueur actuel et son adversaire doivent passer leur tour */
			retour = 3;   /* ca veut dire que la partie est terminee */
			coulact = noir;
		}
		else
			retour = coulact;  /* sinon, on previent que le joueur actuel passe son tour */
	}
	else {
		copieTableau(&tableau, &etatPrecedent);  /* on sauvegarde l'etat precedent du tableau */
		if (coulact == blanc) { /* on fait jouer l'ordinateur ou l'utilisateur, suivant le cas rencontre */
			if (jblordi)
				IA(&caseJouee);
			else {
				if (playerAction)
					caseJouee = playedCoords;
			}
		}
		if (coulact == noir) {
			if (jnoordi)
				IA(&caseJouee);
			else {
				if (playerAction)
					caseJouee = playedCoords;
			}
		}
		if (caseJouee.h != 0) {
			Joue(caseJouee, coulact);  /* on effectue le coup selectionne */
			/* Clignotte */
			retour = 2;
		}
		
	}
	if (retour)
		coulact = -coulact;
	return retour;
}
