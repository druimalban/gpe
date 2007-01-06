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
typedef union {
	long	lval;
	char	cval[255];
} YYSTYPE;
extern YYSTYPE yylval;
