#ifndef _MOTEUR
#define _MOTEUR

#include "globals.h"

void initAtaxx();
/* retourne :
 * 3 si la partie est termin�e
 * 2 clignotte
 * 1 ou -1 : le joueur passe son tour
 * 0 : pas de coup jou�
 */
char Moteur (Coords sel, Coords playedCoords);

#endif /* _MOTEUR */
