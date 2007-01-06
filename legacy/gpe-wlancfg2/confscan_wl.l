%{

#include <math.h>
#include "config-parser.h"
#include "confpars_wl.h"
#include <string.h>

char	tempval[255];
int	linenr=1;
int	sectionstart=0;

%}

digit		[0-9]
letter		[A-Za-z]
special		[$._/\!§%&^]
equals		[=]
quotes		["]
MACSpacer       [:]
caseclose       ";;"
colon		[,]
braceopen	[(]
braceclose	[)]
wildcard        [*]
minus		[-]
pipe		[|]
space		[ ]

%%

case		{ yylval.lval = linenr;return(TOK_CASE);    }	/* case                   */
esac    	{ yylval.lval = linenr;return(TOK_ESAC);    }	/* case-end               */
INFO		{ yylval.lval = linenr;return(TOK_INFO);    }	/* info identifier        */
ESSID		{ yylval.lval = linenr;return(TOK_ESSID);   }	/* essid identifier       */
NWID		{ yylval.lval = linenr;return(TOK_NWID);    }	/* nwid identifier        */
MODE		{ yylval.lval = linenr;return(TOK_MODE);    }	/* mode identifier        */
FREQ		{ yylval.lval = linenr;return(TOK_FREQ);    }	/* freq identifier        */
CHANNEL		{ yylval.lval = linenr;return(TOK_CHANNEL);    }	/* nwid identifier        */
SENS		{ yylval.lval = linenr;return(TOK_SENS);    }	/* sens identifier        */
RATE		{ yylval.lval = linenr;return(TOK_RATE);    } 	/* rate identifier        */
KEY		{ yylval.lval = linenr;return(TOK_KEY);     } 	/* key identifier         */
RTS		{ yylval.lval = linenr;return(TOK_RTS);     } 	/* rts identifier         */
FRAG		{ yylval.lval = linenr;return(TOK_FRAG);    } 	/* frag identifier        */
IWCONFIG	{ yylval.lval = linenr;return(TOK_IWCONFIG);} 	/* ifconfig identifier    */
IWSPY		{ yylval.lval = linenr;return(TOK_IWSPY);   } 	/* iwspy identifier       */
IWPRIV		{ yylval.lval = linenr;return(TOK_IWPRIV);  } 	/* iwpriv identifier      */
in		{ yylval.lval = linenr;return(TOK_IN);      } 	/* in identifier          */

{quotes}	{ yylval.lval = linenr;return(TOK_QUOTES);  }	/* quote char             */
{equals}	{ yylval.lval = linenr;return(TOK_EQUALS);  }	/* assertion              */
{colon}		{ yylval.lval = linenr;return(TOK_COLON);   }	/* colon                  */
{braceclose}    { yylval.lval = linenr;return(TOK_BRACE);   }	/* closing brace          */
{wildcard}      { yylval.lval = linenr;return(TOK_WILDCARD);}	/* wildcard star          */
{caseclose}     { yylval.lval = linenr;return(TOK_SECTEND); }	/* case section end       */
{pipe}		{ yylval.lval = linenr;return(TOK_PIPE);    }   /* pipe-symbol		  */

({minus}|{letter}|{digit}|{special}|{wildcard}|{MACSpacer})+{colon}({minus}|{letter}|{digit}|{special}|{wildcard}|{MACSpacer})+{colon}({minus}|{letter}|{digit}|{special}|{wildcard}|{MACSpacer})+{colon}({minus}|{letter}|{digit}|{special}|{wildcard}|{MACSpacer})+	{strncpy(yylval.cval, yytext, 255); sectionstart=linenr; return(TOK_CASEVAL);}

{quotes}({minus}|{special}|{letter}|{digit}|{colon}|{braceopen}|{braceclose}|{MACSpacer}|{space})*{quotes}	{strncpy(tempval, yytext, 255); strncpy(yylval.cval, &tempval[1], strlen(tempval)-2); yylval.cval[strlen(tempval)-2]=0; return(TOK_TEXTVAL);}

{quotes}?({minus}|{special}|{letter}|{digit}|{space}|{MACSpacer}|" ")*"["{digit}"]"{space}*("key"|"open"|"shared"|"off")*{space}*{quotes}?		{strncpy(yylval.cval, yytext, 255); return(TOK_KEYVAL);}




[\n]		{ linenr++;                              }
[ \t\f\r]	;						/* whitespace             */
"#".*		;						/* comment                */
.		{ printf("Lexer: illegal input\n");      }	/* error                  */

%%

int wl_get_next_token(void)
{
	return(yylex());
}

void wl_set_inputfile(FILE *inputfile)
{
	yyin = inputfile;
	yyrestart(inputfile);
}
