#ifndef _GAMEUTILS
#define _GAMEUTILS

#include "globals.h"

void initTableau ();
short compter (short couleur);
char canPlay (Coords ou, short couleur);
char passe (short couleur);
void Joue (Coords ou, short couleur);
void copieTableau (Grille *source, Grille *dest);

#endif /* _GAMEUTILS */
