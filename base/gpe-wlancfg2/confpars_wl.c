#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"


#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <string.h>

#include "config-parser.h"

Scheme_t schemelist[MAX_SCHEMES];
int	schemecount = 0;

char	schemes[10][32];
int	nrschemes = 0;
int 	count;
int	lcount;

#define DEBUG	0

typedef union {
	long	lval;
	char	cval[255];
} YYSTYPE;
#define TOK_CASE 1000
#define TOK_ESAC 1001
#define TOK_INFO 1002
#define TOK_ESSID 1003
#define TOK_NWID 1004
#define TOK_MODE 1005
#define TOK_FREQ 1006
#define TOK_CHANNEL 1007
#define TOK_SENS 1008
#define TOK_RATE 1009
#define TOK_KEY 1010
#define TOK_RTS 1011
#define TOK_FRAG 1012
#define TOK_IWCONFIG 1013
#define TOK_IWSPY 1014
#define TOK_IWPRIV 1015
#define TOK_IN 1016
#define TOK_EQUALS 1017
#define TOK_COLON 1018
#define TOK_WILDCARD 1019
#define TOK_SECTEND 1020
#define TOK_BRACE 1021
#define TOK_QUOTES 1022
#define TOK_TEXTVAL 1023
#define TOK_CASEVAL 1024
#define TOK_PIPE 1025
#define TOK_KEYVAL 1026
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    1,    1,    3,    4,    4,    5,    6,    6,
    7,    7,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    8,    8,
};
short yylen[] = {                                         2,
    1,    2,    1,    1,    5,    1,    2,    4,    1,    3,
    1,    2,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    1,    2,
};
short yydefred[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    1,    3,    4,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    2,    0,   13,   14,   15,   16,
   17,   18,   19,   20,   21,   28,    0,   23,   24,   25,
   26,   27,    9,    0,    6,    0,   29,    5,    7,    0,
    0,   11,    0,   10,    8,   12,
};
short yydgoto[] = {                                      16,
   17,   18,   19,   54,   55,   56,   63,   47,
};
short yysindex[] = {                                   -978,
-1005, -963, -961, -960, -959, -958, -957, -956, -955, -954,
 -953, -952, -951, -950, -949, -978,    0,    0,    0, -947,
 -970, -948, -946, -945, -944, -943, -942, -941,-1007, -940,
 -939, -938, -937, -936,    0, -935,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -934,    0,    0,    0,
    0,    0,    0,-1001,    0,-1004,    0,    0,    0, -964,
 -933,    0,-1000,    0,    0,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    1,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
   54,   -8,    0,    0,   17,    0,    0,    0,
};
#define YYTABLESIZE 1021
short yytable[] = {                                      58,
   22,    2,    3,    4,    5,    6,    7,    8,    9,   10,
   11,   12,   13,   14,   15,   45,   60,   20,   46,   65,
   61,    1,   53,    2,    3,    4,    5,    6,    7,    8,
    9,   10,   11,   12,   13,   14,   15,    2,    3,    4,
    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
   15,   62,   37,   21,   66,   22,   23,   24,   25,   26,
   27,   28,   29,   30,   31,   32,   33,   34,   36,   35,
   59,    0,    0,    0,   38,    0,   39,   40,   41,   42,
   43,   44,   48,   49,   50,   51,   52,    0,   53,    0,
   64,   57,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   22,    0,   22,   22,   22,   22,   22,   22,   22,   22,
   22,   22,   22,   22,   22,   22,    0,    0,    0,    0,
   22,
};
short yycheck[] = {                                    1001,
    0, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010,
 1011, 1012, 1013, 1014, 1015, 1023, 1021, 1023, 1026, 1020,
 1025, 1000, 1024, 1002, 1003, 1004, 1005, 1006, 1007, 1008,
 1009, 1010, 1011, 1012, 1013, 1014, 1015, 1002, 1003, 1004,
 1005, 1006, 1007, 1008, 1009, 1010, 1011, 1012, 1013, 1014,
 1015,   60, 1023, 1017,   63, 1017, 1017, 1017, 1017, 1017,
 1017, 1017, 1017, 1017, 1017, 1017, 1017, 1017, 1016,   16,
   54,   -1,   -1,   -1, 1023,   -1, 1023, 1023, 1023, 1023,
 1023, 1023, 1023, 1023, 1023, 1023, 1023,   -1, 1024,   -1,
 1024, 1026,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
 1000,   -1, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009,
 1010, 1011, 1012, 1013, 1014, 1015,   -1,   -1,   -1,   -1,
 1020,
};
#define YYFINAL 16
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 1026
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"TOK_CASE","TOK_ESAC","TOK_INFO","TOK_ESSID","TOK_NWID","TOK_MODE",
"TOK_FREQ","TOK_CHANNEL","TOK_SENS","TOK_RATE","TOK_KEY","TOK_RTS","TOK_FRAG",
"TOK_IWCONFIG","TOK_IWSPY","TOK_IWPRIV","TOK_IN","TOK_EQUALS","TOK_COLON",
"TOK_WILDCARD","TOK_SECTEND","TOK_BRACE","TOK_QUOTES","TOK_TEXTVAL",
"TOK_CASEVAL","TOK_PIPE","TOK_KEYVAL",
};
char *yyrule[] = {
"$accept : statement_list",
"statement_list : statement",
"statement_list : statement_list statement",
"statement : assignment",
"statement : case_statement",
"case_statement : TOK_CASE TOK_TEXTVAL TOK_IN caselist TOK_ESAC",
"caselist : case_section",
"caselist : caselist case_section",
"case_section : case_val_list TOK_BRACE value_list TOK_SECTEND",
"case_val_list : TOK_CASEVAL",
"case_val_list : case_val_list TOK_PIPE TOK_CASEVAL",
"value_list : assignment",
"value_list : value_list assignment",
"assignment : TOK_INFO TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_ESSID TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_NWID TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_MODE TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_FREQ TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_CHANNEL TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_SENS TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_RATE TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_KEY TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_KEY TOK_EQUALS keyval_list",
"assignment : TOK_RTS TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_FRAG TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_IWCONFIG TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_IWSPY TOK_EQUALS TOK_TEXTVAL",
"assignment : TOK_IWPRIV TOK_EQUALS TOK_TEXTVAL",
"keyval_list : TOK_KEYVAL",
"keyval_list : keyval_list TOK_KEYVAL",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE

void addkey(char *keystr)
{
	int 	count=0;
	int	tempcount=0;

	char 	tempstr[128];
	char	tempkey1[64];
	char	tempkey2[64];
	char	tempkey3[64];
	char    tempkey4[64];
	char    tempkey5[64];
	char    *temp;
	int	keynr;
	int	pos;

	memset(tempkey1, 0, sizeof(tempkey1));
	memset(tempkey2, 0, sizeof(tempkey2));
	memset(tempkey3, 0, sizeof(tempkey3));
	memset(tempkey4, 0, sizeof(tempkey4));
	memset(tempkey5, 0, sizeof(tempkey5));
	sprintf(schemelist[schemecount].Encryption, "on");
	sprintf(schemelist[schemecount].EncMode, "shared");
	sprintf(schemelist[schemecount].ActiveKey, "1");

	while((keystr[count]==' ') || keystr[count]=='"') count++;
	while((count<strlen(keystr)) && (keystr[count]!='"') && (keystr[count]!=0))
	{
		tempstr[tempcount++]=keystr[count++];
	}
	tempstr[tempcount]=0;

	if (strlen(tempstr)==0)
	{
		if (DEBUG==1) printf("yacc: KEY = No Keys found\n");
		sprintf(schemelist[schemecount].Encryption, "off");
		return;
	}
	temp = strtok(tempstr, " ");
	if (temp!=NULL) strncpy(tempkey1, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey2, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey3, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey4, temp, 64);

	temp = strtok(NULL, " ");
	if (temp!=NULL) strncpy(tempkey5, temp, 64);

	if (tempkey1[0]=='[')
	{
		sprintf(schemelist[schemecount].ActiveKey, "%i", tempkey1[1]-'0');
		if (strcmp(tempkey2, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey2, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
		}
		if (strcmp(tempkey2, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
		}

		return;
	}

	if (strlen(tempkey2)==0)
	{
		if (strcmp(tempkey1, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey1, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
			return;
		}
		if (strcmp(tempkey1, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
			return;
		}

		keynr = 1;
	}

	keynr = tempkey2[1]-'0';

	if (strlen(tempkey3)==0)
	{
		if (strcmp(tempkey2, "off") == 0) sprintf(schemelist[schemecount].Encryption, "off");
		if (strcmp(tempkey2, "open") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "open");
		}
		if (strcmp(tempkey2, "shared") == 0)
		{
			sprintf(schemelist[schemecount].Encryption, "on");
			sprintf(schemelist[schemecount].EncMode, "shared");
		}

		keynr = 1;
	}

	pos = 0;
	if (tempkey1[1]==':')
	{
		pos=2;
		schemelist[schemecount].KeyFormat=TRUE;
	}
	switch(keynr)
	{
		case 1: strncpy(schemelist[schemecount].key1, &tempkey1[pos], 64); break;
		case 2: strncpy(schemelist[schemecount].key2, &tempkey1[pos], 64); break;
		case 3: strncpy(schemelist[schemecount].key3, &tempkey1[pos], 64); break;
		case 4: strncpy(schemelist[schemecount].key4, &tempkey1[pos], 64); break;
	}
}

void yyerror(char *errmsg)
{
	fprintf(stderr, "Scanner: %s\n", errmsg);
	input_file_error = TRUE;
}

int parse_input(void)
{
	return(yyparse());
}
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 5:
{	esac_line = yyvsp[0].lval;	}
break;
case 8:
{
								schemelist[schemecount].scheme_start = sectionstart;
								schemelist[schemecount].scheme_end   = yyvsp[0].lval;

								/*strncpy(schemelist[schemecount].Scheme, $1, 32);*/
								schemelist[schemecount].lines[L_Scheme]=sectionstart;
								schemelist[schemecount].lines[L_Socket]=sectionstart;
								schemelist[schemecount].lines[L_Instance]=sectionstart;
								schemelist[schemecount].lines[L_HWAddress]=sectionstart;

								for (count=0; count<nrschemes; count++)
								{
									if (count>0) memcpy(&schemelist[schemecount+count], &schemelist[schemecount], sizeof(Scheme_t));
									strncpy(schemelist[schemecount+count].Scheme, strtok(schemes[count] ,","), 32);
									strncpy(schemelist[schemecount+count].Socket, strtok(NULL ,","), 32);
									strncpy(schemelist[schemecount+count].Instance, strtok(NULL ,","), 32);
									strncpy(schemelist[schemecount+count].HWAddress, strtok(NULL ,","), 32);
									
									if (count>0)
									{
										schemelist[schemecount+count].parent_scheme_end = schemelist[schemecount].scheme_end;
										for (lcount=0; lcount<MAX_CONFIG_VALUES; lcount++)
											if (schemelist[schemecount+count].lines[lcount])
												schemelist[schemecount+count].lines[lcount]=LINE_NEW;
										
											
									}

									if (DEBUG==1) printf("yacc: Case section detected: %s,%s,%s,%s\n",
											schemelist[schemecount+count].Scheme,
											schemelist[schemecount+count].Socket,
											schemelist[schemecount+count].Instance,
				 							schemelist[schemecount+count].HWAddress);
								}

								schemecount+=nrschemes;
								memset(schemes, 0, sizeof(schemes));
								nrschemes=0;
							}
break;
case 9:
{	strncpy(schemes[nrschemes], yyvsp[0].cval, 32); nrschemes++; }
break;
case 10:
{	strncpy(schemes[nrschemes], yyvsp[0].cval, 32); nrschemes++; }
break;
case 13:
{
								strncpy(schemelist[schemecount].Info, yyvsp[0].cval, 64);
								schemelist[schemecount].lines[L_Info]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: INFO = %s\n", yyvsp[0].cval);
							}
break;
case 14:
{
								strncpy(schemelist[schemecount].ESSID, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_ESSID]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: ESSID = %s\n", yyvsp[0].cval);
							}
break;
case 15:
{
								strncpy(schemelist[schemecount].NWID, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_NWID]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: NWID = %s\n", yyvsp[0].cval);
							}
break;
case 16:
{
								strncpy(schemelist[schemecount].Mode, yyvsp[0].cval, 16);
								schemelist[schemecount].lines[L_Mode]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: MODE = %s\n", yyvsp[0].cval);

							}
break;
case 17:
{
								strncpy(schemelist[schemecount].Frequency, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_Frequency]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: FREQUENCY = %s\n", yyvsp[0].cval);
							}
break;
case 18:
{
								strncpy(schemelist[schemecount].Channel, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_Channel]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: CHANNEL = %s\n", yyvsp[0].cval);
							}
break;
case 19:
{
								strncpy(schemelist[schemecount].sens, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_sens]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: SENS = %s\n", yyvsp[0].cval);	
							}
break;
case 20:
{
								strncpy(schemelist[schemecount].Rate, yyvsp[0].cval, 8);
								schemelist[schemecount].lines[L_Rate]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: RATE = %s\n", yyvsp[0].cval);
							}
break;
case 21:
{
								addkey(yyvsp[0].cval);
								if (DEBUG==1) printf("Evaluated Keys:\n  Key1: %s\n  Key2: %s\n  Key3: %s\n  Key4: %s\n  Active: %s\n  Mode: %s\n  Encryption: %s\n\n",
								schemelist[schemecount].key1,
								schemelist[schemecount].key2,
								schemelist[schemecount].key3,
								schemelist[schemecount].key4,
								schemelist[schemecount].ActiveKey,
								schemelist[schemecount].EncMode,
								schemelist[schemecount].Encryption);
								
								schemelist[schemecount].lines[L_key1] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key2] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key3] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key4] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_ActiveKey]  = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_EncMode]    = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_Encryption] = yyvsp[-2].lval;

							}
break;
case 22:
{
								if (DEBUG==1) printf("Evaluated Keys:\n  Key1: %s\n  Key2: %s\n  Key3: %s\n  Key4: %s\n  Active: %s\n  Mode: %s\n  Encryption: %s\n\n",
								schemelist[schemecount].key1,
								schemelist[schemecount].key2,
								schemelist[schemecount].key3,
								schemelist[schemecount].key4,
								schemelist[schemecount].ActiveKey,
								schemelist[schemecount].EncMode,
								schemelist[schemecount].Encryption);
								
								schemelist[schemecount].lines[L_key1] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key2] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key3] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_key4] = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_ActiveKey]  = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_EncMode]    = yyvsp[-2].lval;
								schemelist[schemecount].lines[L_Encryption] = yyvsp[-2].lval;
							}
break;
case 23:
{
								strncpy(schemelist[schemecount].rts, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_rts]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: RTS = %s\n", yyvsp[0].cval);	
							}
break;
case 24:
{
								strncpy(schemelist[schemecount].frag, yyvsp[0].cval, 32);
								schemelist[schemecount].lines[L_frag]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: FRAG = %s\n", yyvsp[0].cval);	
							}
break;
case 25:
{
								strncpy(schemelist[schemecount].iwconfig, yyvsp[0].cval, 255);
								schemelist[schemecount].lines[L_iwconfig]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: IWCONFIG = %s\n", yyvsp[0].cval);
							}
break;
case 26:
{
								strncpy(schemelist[schemecount].iwspy, yyvsp[0].cval, 255);
								schemelist[schemecount].lines[L_iwspy]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: IWSPY = %s\n", yyvsp[0].cval);
							}
break;
case 27:
{
								strncpy(schemelist[schemecount].iwpriv, yyvsp[0].cval, 255);
								schemelist[schemecount].lines[L_iwpriv]=yyvsp[-2].lval;
								if (DEBUG==1) printf("yacc: IWPRIV = %s\n", yyvsp[0].cval);
							}
break;
case 28:
{	addkey(yyvsp[0].cval);	}
break;
case 29:
{	addkey(yyvsp[0].cval);	}
break;
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
