#include "gameutils.h"
#include "globals.h"


typedef struct {
char N, S, E, W, NE, NW, SE, SW;
} Direction;


void initTableau ()  /* initialise le tableau et place les 4 pions de depart */
{
	short i, j;
	
	for (j = 0 ; j <= haut + 1 ; j++)
		for (i = 0 ; i <= larg + 1 ; i++)
			tableau[i][j] = 0;
	tableau[1][1] = blanc;
	tableau[larg][1] = noir;
	tableau[1][haut] = noir;
	tableau[larg][haut] = blanc;
}

short compter_board (Grille t, short couleur)  /* retourne le nombre de pions d'une couleur donnee */
{
	short i, j, comp;
	
	comp = 0;
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i<= larg ; i++)
			if (t[i][j] == couleur)
				comp ++;
	return comp;
}
short compter (short couleur)  /* retourne le nombre de pions d'une couleur donnee */
{
	short i, j, comp;
	
	comp = 0;
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i<= larg ; i++)
			if (tableau[i][j] == couleur)
				comp ++;
	return comp;
}


/* cette procedure retourne les directions jouables pour un coup d'une couleur donnee a des coordonnees donnees */
void analyse (Coords ou, short couleur, Direction *theDirection)
{
	short i, j;
	char imposs;
	
	if (tableau[ou.h][ou.v] != vide) ;
	else {
		theDirection->N = FALSE;
		theDirection->S = FALSE;
		theDirection->E = FALSE;
		theDirection->W = FALSE;
		theDirection->NE = FALSE;
		theDirection->NW = FALSE;
		theDirection->SE = FALSE;
		theDirection->SW = FALSE;

		imposs = FALSE; /* direction E */
		i = ou.h + 1;
		j = ou.v;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i++;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->E = TRUE;
			}
		}

		imposs = FALSE; /* direction W */
		i = ou.h - 1;
		j = ou.v;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i--;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->W = TRUE;
			}
		}

		imposs = FALSE; /* direction N */
		i = ou.h;
		j = ou.v - 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			j--;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->N = TRUE;
			}
		}

		imposs = FALSE; /* direction S */
		i = ou.h;
		j = ou.v + 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			j++;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->S = TRUE;
			}
		}

		imposs = FALSE; /* direction SE */
		i = ou.h + 1;
		j = ou.v + 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i++;
			j++;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->SE = TRUE;
			}
		}

		imposs = FALSE; /*direction NE */
		i = ou.h + 1;
		j = ou.v - 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i++;
			j--;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->NE = TRUE;
			}
		}
		
		imposs = FALSE; /* direction SW */
		i = ou.h - 1;
		j = ou.v + 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i--;
			j++;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->SW = TRUE;
			}
		}

		imposs = FALSE; /* direction NW */
		i = ou.h - 1;
		j = ou.v - 1;
		if (tableau[i][j] != (-couleur))
			imposs = TRUE;
		while (!imposs) {
			i--;
			j--;
			if (tableau[i][j] == vide)
				imposs = TRUE;
			if (tableau[i][j] == couleur) {
				imposs = TRUE;
				theDirection->NW = TRUE;
			}
		}
	}
}



/* indique la possibilite de jouer a un emplacement donne */
/* cette fonction ressemble beaucoup a Analyse, mais elle arrete son investigation */
/* des qu'elle a trouve une direction jouable, ce qui la rend plus rapide */
char canPlay (Coords sel, Coords ou, short couleur)
{
	if (!(sel.v > 0 && sel.v <= 8 &&
			sel.h > 0 && sel.h <= 8 &&
			ou.v > 0 && ou.v <= 8 &&
			ou.h > 0 && ou.h <= 8))
		return 0;
	if (abs(sel.v - ou.v) <= 1 && abs(sel.h - ou.h) <= 1 && tableau[ou.h][ou.v] == 0) {
		return 1;
	} else 	if (abs(sel.v - ou.v) <= 2 && abs(sel.h - ou.h) <= 2 && tableau[ou.h][ou.v] == 0) {
		return 2;
	}
	
	return 0;
}


/* Modifie le tableau pour effectuer le coup joue */
void Joue_board (Coords sel, Coords ou, short couleur, Grille *tableau)
{
	int x,y;

	/* Clear the original spot if it's a jump */
	if (canPlay (sel, ou, couleur) == 2) {
		(*tableau)[sel.h][sel.v] = 0;
	}

	/* Move / Clone */
	(*tableau)[ou.h][ou.v] = couleur;

	/* Infect surrounding cells */
	for (x=ou.h-1;x<ou.h+2;x++)
		for (y=ou.v-1;y<ou.v+2;y++)
			if ((*tableau)[x][y] != 0)
				(*tableau)[x][y] = couleur;
}

/* Modifie le tableau pour effectuer le coup joue */
void Joue (Coords sel, Coords ou, short couleur)
{
	Joue_board (sel, ou, couleur, &tableau);
#if 0
	int x,y;

	/* Clear the original spot if it's a jump */
	if (canPlay (sel, ou, couleur) == 2) {
		tableau[sel.h][sel.v] = 0;
	}

	tableau[ou.h][ou.v] = couleur;

	/* Move / Clone */
	for (x=ou.h-1;x<ou.h+2;x++)
		for (y=ou.v-1;y<ou.v+2;y++)
			if (tableau[x][y] != 0)
				tableau[x][y] = couleur;
#endif
}


/* procedure utilitaire de copie de tableau */
void copieTableau (Grille *source, Grille *dest)
{
	short i, j;
	
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i <= larg ; i++)
			(*dest)[i][j] = (*source)[i][j];
}
