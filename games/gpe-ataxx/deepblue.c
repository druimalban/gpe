#include <stdlib.h>
#include "globals.h"
#include "gameutils.h"
#include "deepblue.h"

void print_tableau (Grille tableau) {
	int x,y;
	for (y=1;y<=8;y++) {
		for (x=1;x<=8;x++) {
			if (tableau[x][y] == noir) printf ("B");
			else if (tableau[x][y] == blanc) printf ("W");
			else printf (".");
		}
		printf ("\n");
	}
}

#if 0
#define	maxcoups  haut * larg
typedef  Coords CoordIndex[maxcoups+1]; /* portage : 1 de trop pour ne pas avoir a reajuster */
typedef short ScoreIndex[maxcoups+1]; /* idem */

short prof;

Grille	valeur;
short	depth;

/*attribue les coefficients de chaque case */
void CalculeValeur()  
{
	short i, j;

	for (i = 1 ; i <= larg ; i++)
		for (j = 1 ; j <= haut ; j++)
			valeur[i][j] = 3;
			
	valeur[1][1] = 20;
	valeur[larg][1] = 20;
	valeur[larg][haut] = 20;
	valeur[1][haut] = 20;
	for (i = 3 ; i<= larg - 2 ; i++)
		valeur[i][1] = 7;
	for (i = 3 ; i<= larg - 2 ; i++)
		valeur[i][haut] = 7;
	for (j = 3 ; j<= haut - 2 ; j++)
		valeur[1][j] = 7;
	for (j = 3 ; j<= haut - 2 ; j++)
		valeur[larg][j] = 7;
	valeur[2][2] = 0;
	valeur[2][haut - 1] = 0;
	valeur[larg - 1][2] = 0;
	valeur[larg - 1][haut - 1] = 0;
	valeur[3][3] = 8;
	valeur[3][haut - 2] = 8;
	valeur[larg - 2][3] = 8;
	valeur[larg - 2][haut - 2] = 8;
	valeur[larg / 2][haut / 2 - 1] = 5;
	valeur[larg / 2 + 1][haut / 2 - 1] = 5;
	valeur[larg / 2 - 1][haut / 2] = 5;
	valeur[larg / 2 - 1][haut / 2 + 1] = 5;
	valeur[larg / 2 + 2][haut / 2] = 5;
	valeur[larg / 2 + 2][haut / 2 + 1] = 5;
	valeur[larg / 2][haut / 2 + 2] = 5;
	valeur[larg / 2 + 1][haut / 2 + 2] = 5;
/* toutes les cases du tableau prennent une valeur, independamment de la taille du tableau */
/* neanmoins, ces coefficients sont appropries a un tableau de 8x8 (taille standard) */
}


short coupsJouables (short couleur, CoordIndex *positions) /* retourne le nb de coups jouables */
{
	Coords test;
	short coupsj, i, j;
	return 0; // MIBUS
#if 0
	coupsj = 0;
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i <= larg ;i++) {
			test.h = i;
			test.v = j;
			if (canPlay(test, couleur)) {
				coupsj++;
				(*positions)[coupsj] = test; /* on remplit egalement la liste des coups jouables */
			}
		}
	return coupsj;
#endif
}


short heuristique (short gentil)  /* evalue l'etat du tableau */
{
	short i, j;
	short score;

	score = 0;
	for (j = 1 ; j <= haut ; j++)
		for (i = 1 ; i <= larg ;i++)
			if (tableau[i][j] == gentil)
				score += valeur[i][j];
			else
				score -= valeur[i][j];
	return score;
}

/* cette fonction determine la valeur et la case a remonter dans l'arborescence */
short determine (short couleur, ScoreIndex index, short quantite, short *bonindex)
{
	short i, result;
	
	if (couleur == coulact)
		result = -32000;
	else
		result = 32000;
		
	for (i = 1 ; i <= quantite ; i++) {
		if (((couleur == coulact) && (index[i] > result)) || ((couleur == -coulact) && (index[i] < result))) {
			result = index[i];
			*bonindex = i;
		}
	}
	return result;
}

short MinMax (short couleur, Coords *bonnecase)  /* fonction recursive d'exploration de l'arbre */
{
	return 0;
#if 0
	Grille backup;
	short nb, i;
	CoordIndex liste;
	ScoreIndex coups;
	short result=0;

	if (prof == depth)              /* si on a atteint la profondeur souhaitŽe, on evalue l'etat du tableau */
		result = heuristique (coulact);
	else {
		nb = coupsJouables (couleur, &liste);
		if (nb > 0)                /* Si on peut jouer, on joue tous les coups possibles */
			for (i = 1 ; i <= nb; i++) {
				copieTableau(&tableau, &backup);
				Joue(liste[i], couleur);
				prof++;
				coups[i] = MinMax(-couleur, bonnecase);
				copieTableau(&backup, &tableau);
				prof--;
			} else {                               /* si il n'y a pas de coup possible, on passe au tour suivant */
				prof++;
				coups[1] = MinMax(-couleur, bonnecase);
				nb = 1;
				prof--;
			}
			result = determine(couleur, coups, nb, &i);   /* La valeur a remonter est donnee par Determine */
			*bonnecase = liste[i];               /* On recupere les coordonnees correspondant a la valeur remontee */
	}
	return result;
#endif
}

void IA (Coords * result)  /* procedure principale de l'intelligence artificielle */
{
	prof = 0;
	CalculeValeur();  /* Attribue les coefficients utilises pour calculer l'heuristique */
	MinMax(coulact, result); /* appel de MinMax (la valeur de retour est ignoree) */
}
#endif

void IA (Coords *sel, Coords *result)
{
	int x,y;
	int biggest=-1;
	// Winners - number of "top" entries so far
	int winners;
	
	/*
	printf ("---- Original board ----\n");
	print_tableau (tableau);
	printf ("---- /Original board ----\n");
	*/
	
	for (y=1;y<=8;y++)
		for (x=1;x<=8;x++)
		{
			if (tableau[x][y] == coulact)
			{
				int x2,y2;
				for (y2=y-2;y2<=y+2;y2++) {
					for (x2=x-2;x2<=x+2;x2++) {
						Grille future;
						Coords from, to;
						
						memcpy (future, tableau, sizeof(Grille));
						
						from.h = x; from.v = y;
						to.h = x2; to.v = y2;
						
						if (canPlay (from, to, coulact)) {
							Joue_board (from, to, coulact, &future);
							/*printf ("\n\n--\n");
							printf ("Move from (%d,%d) -> (%d,%d)\n", x,y, x2,y2);
							printf ("%d pieces (from %d)\n", compter_board (future, coulact), compter_board (tableau, coulact));
							printf ("(biggest = %d)\n", biggest);
							printf ("---- New board ----\n");
							print_tableau (future);
							printf ("---- /New board ----\n");*/
							if (compter_board (future, coulact) >= biggest) {
								// If we're a better "winner", reset the winners numbers
								if (biggest == compter_board (future, coulact)) {
									// Randomly ignore results
									if (rand() % (winners++) > 0) { /*printf ("Ignored potential winner\n"); */ break; }
								} else
									winners = 1;
								//printf ("We have a winner! %d,%d -> %d,%d\n", x,y, x2,y2);
								biggest = compter_board (future, coulact);
								sel->h = x;
								sel->v = y;
								result->h = x2;
								result->v = y2;
							}
						}
					
					}
				}
				
			}
		}
}
