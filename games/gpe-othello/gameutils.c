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
	tableau[larg / 2][haut / 2] = blanc;
	tableau[larg / 2 + 1][haut / 2] = noir;
	tableau[larg / 2][haut / 2 + 1] = noir;
	tableau[larg / 2 + 1][haut / 2 + 1] = blanc;
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
char canPlay (Coords ou, short couleur)
{
	short i, j;
	char imposs, bon;
	char result;
	
	if (tableau[ou.h][ou.v] != vide)
		result = FALSE;
	else {
		bon = FALSE;

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
				bon = TRUE;
				imposs = TRUE;
			}
		}

		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}
		
		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}


		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}


		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}


		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}


		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}


		if (!bon) {
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
					bon = TRUE;
					imposs = TRUE;
				}
			}
		
		} } } } } } }
		
		result = bon;
	}
	return result;
}

/* determine si un joueur d'une couleur donnee doit passer son tour */
char passe (short couleur)
{
	Coords test;
	short coups, i, j;
	
	coups = 0;
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i <= larg ; i++) {
			test.h = i;
			test.v = j;
			if (canPlay (test, couleur))
				coups++;
		}
		
	if (coups > 0)
		return FALSE;
	else
		return TRUE;
}


/* Modifie le tableau pour effectuer le coup joue */
void Joue (Coords ou, short couleur)
{
	short i=0, j=0;
	Direction theDirection;
	
	analyse (ou, couleur, &theDirection);
	if (theDirection.N) {
		i = ou.h;
		j = ou.v - 1;
		while (tableau[i][j] != couleur) {
			if (j > 0)
				tableau[i][j] = couleur;
			j--;
		}
	}

	if (theDirection.S) {
		i = ou.h;
		j = ou.v + 1;
		while (tableau[i][j] != couleur) {
			if (j < xhaut)
				tableau[i][j] = couleur;
			j++;
		}
	}

	if (theDirection.E) {
		i = ou.h + 1;
		j = ou.v;
		while (tableau[i][j] != couleur) {
			if (i < xlarg)
				tableau[i][j] = couleur;
			i++;
		}
	}

	if (theDirection.W) {
		i = ou.h - 1;
		j = ou.v;
		while (tableau[i][j] != couleur) {
			if (i > 0)
				tableau[i][j] = couleur;
			i--;
		}
	}

	if (theDirection.NE) {
		i = ou.h + 1;
		j = ou.v - 1;
		while (tableau[i][j] != couleur) {
			if ((j > 0) && (i < xlarg))
				tableau[i][j] = couleur;
			j--;
			i++;
		}
	}

	if (theDirection.NW) {
		i = ou.h - 1;
		j = ou.v - 1;
		while (tableau[i][j] != couleur) {
			if ((j > 0) && (i > 0))
				tableau[i][j] = couleur;
			j--;
			i--;
		}
	}

	if (theDirection.SE) {
		i = ou.h + 1;
		j = ou.v + 1;
		while (tableau[i][j] != couleur) {
			if ((j < xhaut) && (i < xlarg))
				tableau[i][j] = couleur;
			j++;
			i++;
		}
	}

	if (theDirection.SW) {
		i = ou.h - 1;
		j = ou.v + 1;
		while (tableau[i][j] != couleur) {
			if ((j < xhaut) && (i > 0))
				tableau[i][j] = couleur;
			j++;
			i--;
		}
	}
	
	if ((j < xhaut) && (i < xlarg) && (j > 0) && (i > 0))
			tableau[ou.h][ou.v] = couleur;

}


/* procedure utilitaire de copie de tableau */
void copieTableau (Grille *source, Grille *dest)
{
	short i, j;
	
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i <= larg ; i++)
			(*dest)[i][j] = (*source)[i][j];
}
