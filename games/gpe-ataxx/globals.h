#ifndef _GLOBALS
#define _GLOBALS


#define		larg 8
#define		haut 8
#define		xlarg larg + 1
#define		xhaut haut + 1
#define		vide 0
#define		blanc 1
#define		noir -blanc

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef struct {
	short h,v;
} Coords;

typedef short Grille[xhaut+1][xlarg+1];

typedef Grille * GrillePtr;


extern short	quit;
extern Grille	tableau, etatPrecedent;
extern short	coulact;
extern Coords	caseJouee;
extern short	jblordi, jnoordi;
extern short	depth;

#endif /* _GLOBALS */

