#ifndef _MOTEUR
#define _MOTEUR

#include "globals.h"

void initOthello();
/* retourne :
 * 3 si la partie est terminée
 * 2 clignotte
 * 1 ou -1 : le joueur passe son tour
 * 0 : pas de coup joué
 */
char Moteur (Coords playedCoords);

#endif /* _MOTEUR */

