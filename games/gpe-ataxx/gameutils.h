#ifndef _GAMEUTILS
#define _GAMEUTILS

#include "globals.h"

void initTableau ();
short compter (short couleur);
short compter_board (Grille tableu, short couleur);
char canPlay (Coords sel, Coords ou, short couleur);
void Joue (Coords sel, Coords ou, short couleur);
void Joue_board (Coords sel, Coords ou, short couleur, Grille *tableau);
void copieTableau (Grille *source, Grille *dest);

#endif /* _GAMEUTILS */
