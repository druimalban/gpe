/* some code for hebrew support under linux
 * didn't have to much time to work hard on it, only few hours
 * use it as you wish, this is subjected to the GPL copyright 
 * and add the standard disclaimer. ( i am not reponisbale for
 * anything that might of this software, use it on your own risk )
 * 8 March 1998, Erez Doron, mailto:erez@savan.com
 * enjoy
 */

#include <stdio.h>
/*#include "ukbhit.h"*/
#ifndef LIB
#include "key.h"
#endif

#define LINE_WIDTH 70

/* first some defines 
 * IS_HEB_CHAR is a macro to check if a character is in hebrew 
 * IS_BIDIR is a macro which checks if a character is not english ( or digit )
 * and also not hebrew i.e. bidir for bidirectional */
#define IS_HEB_CHAR(c)	(c>=224 && c<=250)
#define IS_BIDIR(c)	(((c<'A' || c>'z' || (c>'Z' && c<'a'))&&(c>'9'||c<'0'))&& !IS_HEB_CHAR(c))

/*
 * defines every letter in the hebrew alphabet and it's ascii
 * according to iso8859-8
 */
#define ALEF		224
#define BET		225
#define GIMEL		226
#define DALET		227
#define HEY		228
#define VAV		229
#define ZAIN		230
#define HET		231
#define TET		232
#define YOD		233
#define KAF_SOFIT	234
#define KAF		235
#define LAMED		236
#define MEM_SOFIT	237
#define MEM		238
#define NUN_SOFIT	239
#define NUN		240
#define SAMEH		241
#define AIN		242
#define PEY_SOFIT	243
#define PEY		244
#define TZADI_SOFIT	245
#define TZADI		246
#define KUF		247
#define RESH		248
#define SHIN		249
#define TAF		250


typedef unsigned char u_char;


char primary_hebrew=0;
char bidir_follows=1;
char bidir_l2r=1;


#ifndef LIB
/*
 * hebrew_key converts a english char to its equivalent
 * in hebrew, for hebrew keyboard mupping.
 * i.e. it translates 'a' to shin, and 's' to dalet
 */

u_char hebrew_key(u_char c)
{
 switch(c)
 {
 case '`'  : c=';';break;
 case 'q'  : c='/';break;
 case 'w'  : c='\'';break;
 case 'e'  : c=KUF;break;
 case 'r'  : c=RESH;break;
 case 't'  : c=ALEF;break;
 case 'y'  : c=TET;break;
 case 'u'  : c=VAV;break;
 case 'i'  : c=NUN_SOFIT;break;
 case 'o'  : c=MEM_SOFIT;break;
 case 'p'  : c=PEY;break;
 case 'a'  : c=SHIN;break;
 case 's'  : c=DALET;break;
 case 'd'  : c=GIMEL;break;
 case 'f'  : c=KAF;break;
 case 'g'  : c=AIN;break;
 case 'h'  : c=YOD;break;
 case 'j'  : c=HET;break;
 case 'k'  : c=LAMED;break;
 case 'l'  : c=KAF_SOFIT;break;
 case ';'  : c=PEY_SOFIT;break;
 case '\'' : c=',';break;
 case 'z'  : c=ZAIN;break;
 case 'x'  : c=SAMEH;break;
 case 'c'  : c=BET;break;
 case 'v'  : c=HEY;break;
 case 'b'  : c=NUN;break;
 case 'n'  : c=MEM;break;
 case 'm'  : c=TZADI;break;
 case ','  : c=TAF;break;
 case '.'  : c=TZADI_SOFIT;break;
 case '/'  : c='.';break;
 }
 return c;
}

#endif
/*
 * heb_l2v1c is used for every char on heb_logical2visual() below
 * it puts a new character at position <r> if right=1,
 * or put it at position <p> otherwise
 * then update <r> and <p> and shift the rest one char to the right
 */

void heb_l2v1c(u_char c,u_char *dst,int *p,int *r,char right)
{
 int i,j;

 if (right)
 {
    j=strlen(dst);
    if (primary_hebrew)
    {
       dst[j+1]=0;
       for(i=j;i>*r;i--) dst[i]=dst[i-1];
       dst[(*r)++]=c;
    }
    else
    {
       dst[j+1]=0;
       dst[j]=c;
       *p=j+1;
    }
 }
 else
 {
    j=strlen(dst);
    dst[j+1]=0;
    for(i=j;i>*p;i--) dst[i]=dst[i-1];
    dst[*p]=c;
    if (primary_hebrew) *r=0;
 }
}

/*
 * heb_logical2visual converts a logical string ( as typed in hebrew )
 * to visual ( to be printed from left to right )
 * THIS IS THE GOAL OF THIS FILE
 * it puts english/digits - left to right
 * it puts hebrew - right to left
 * it puts other chars from left to right (english) unless it
 * is between 2 hebrew ones ( may be some bidir chars between hebrew )
 */

void l2v(char *src0,char *dst0)
{
 int add2right;
 int i,j,r,p;
 unsigned char *src;
 unsigned char *dst;
 src=src0;
 dst=dst0;

 r=p=0;
 add2right=!primary_hebrew;
 dst[0]=0;
 for(i=0;src[i];i++)
 {
    if (IS_HEB_CHAR(src[i]))
    {
       add2right=0;
       heb_l2v1c(src[i],dst,&p,&r,add2right);
    }
    else
    {
       if (!IS_BIDIR(src[i]))
       {
          add2right=1;
	  heb_l2v1c(src[i],dst,&p,&r,add2right);
       }
       else /*bidir*/
       {
	 /* search for not bidir or end */
         for(j=i+1;src[j] && IS_BIDIR(src[j]);j++);
	 if (!src[j]) add2right=!primary_hebrew;
	 else if (add2right==IS_HEB_CHAR(src[j])||!bidir_follows)
	   add2right=bidir_l2r;
	 else if (primary_hebrew) 
	 {
            if (src[j]) add2right&=!IS_HEB_CHAR(src[j]);
	 }
	 else
	 {
            if (src[j]) add2right|=!IS_HEB_CHAR(src[j]);
	 }
         for(;i<j;i++) heb_l2v1c(src[i],dst,&p,&r,add2right);
	 i--;
       }
    }
 }
}

void l2vml(char *src0,char *dst0)
{
   char *sp;
   char *sp2;

   *dst0=0;
   while (NULL!=(sp=(char *)strchr(src0,'\n')))
   {
      *sp=0;
      l2v(src0,dst0);
      src0+=strlen(src0);
      *(src0++)='\n';
      dst0+=strlen(dst0);
      *(dst0++)='\n';
   }
   l2v(src0,dst0);
}

#ifndef LIB
char my_getch()
{
 char c;
 set_keypress();
 c=getchar();
 reset_keypress();
 return c;
}
#endif



/* main reads a char, convert it as for hebrew keyboard
 * ( by hebrew_key() ) then appends it to a string, convert it
 * from logical to visual, and prints it
 */

#ifndef LIB
void main(int argc,char **argv)
{
 u_char c;
 u_char s[1000]="";
 u_char s2[2000];
 int i;
 char *argv0=*argv;

 char piped = 0;
 char translate = 1;
 char params=0;
 char pad=0;




 primary_hebrew=1;
 bidir_follows=1;
 bidir_l2r=1;
 argv++;
 while (argc>1)
 {
  params=1;
  if (0==strcmp(*argv,"-h")) primary_hebrew=0;
  else if (0==strcmp(*argv,"+h")) primary_hebrew=1;
  else if (0==strcmp(*argv,"-t")) translate=0;
  else if (0==strcmp(*argv,"+t")) translate=1;
  else if (0==strcmp(*argv,"-p")) pad=0;
  else if (0==strcmp(*argv,"+p")) pad=1;
  else if (0==strcmp(*argv,"+l")) piped=1;
  else if (0==strcmp(*argv,"+c")) piped=0;
  else if (0==strcmp(*argv,"+2f")) bidir_follows=1;
  else if (0==strcmp(*argv,"-2f")) bidir_follows=0;
  else if (0==strcmp(*argv,"2r")) bidir_l2r=1;
  else if (0==strcmp(*argv,"2l")) bidir_l2r=0;
  else if (0==strcmp(*argv,"ch")) {
       primary_hebrew=translate=pad=bidir_follows=1;
       bidir_l2r=piped=0;
       }
  else if (0==strcmp(*argv,"ce")) {
       translate=pad=bidir_follows=bidir_l2r=1;
       primary_hebrew=piped=0;
       }
  else if (0==strcmp(*argv,"pe")) {
       piped=bidir_follows=bidir_l2r=1;
       pad=translate=primary_hebrew=0;
       }
  else params=0;
  argc--;
  argv++;
 }
 if (!params) 
 {
   printf("Logical to visual Hebrew converter V1.1\n");
   printf("(C) 1998 Erez Doron - erez@savan.com\n\n");
   printf("Syntax: %s <switches>\n\n",argv0);
   printf("Swithces:\n");
   printf("        +h  primary is hebrew (right to left)\n");
   printf("        -h  primary is not hebrew\n");
   printf("        +t  translate for hebrew keyboard\n");
   printf("        -t  do not translate for hebrew keyboard\n");
   printf("        +p  pad lines(align to right) also split if too long\n");
   printf("            (works only with +l)\n");
   printf("        -p  do not pad lines \n");
   printf("        +l  read line at a time\n");
   printf("        +c  read char at a time\n");
   printf("        2r  bidirectional left to right\n");
   printf("        2l  bidirectional right to left\n");
   printf("        +2f bidirectional follows majority\n");
   printf("        -2f bidirectional does not follow majority\n");
   printf("        ch  for console with dominant hebrew  - same as +h 2l +t +p +c +2f\n");
   printf("        ce  for console with dominant english - same as +h 2l +t +p +c +2f\n");
   printf("        pe  piped enviroment, as filter       - same as -h 2r -t -p +l +2f\n");
   return;
 }
 
 
 if (piped) 
 {
    i=0;
    while (!feof(stdin)) 
    {
       while (!feof(stdin)&&(c=getchar())!='\n' && (i<LINE_WIDTH || !pad))
       {
          if (translate) c=hebrew_key(c);
	  s[i++]=c;
       }
       s[i]=0;
       heb_logical2visual(s,s2);
       if (pad) for(i=strlen(s2);i<LINE_WIDTH;i++) printf(" ");
       printf("%s\n",s2);
       i=0;
       if (translate) c=hebrew_key(c);
       if (c!='\n') s[i++]=c;
    }
    return; 
 }
 while ((c=my_getch())!=27) {
  if (translate) c=hebrew_key(c);
  i=strlen(s);
  s[i]=c;
  s[i+1]=0;
  if (c==10) 
  {
     printf("\n");
     s[0]=0;
  }
  else
  {
     heb_logical2visual(s,s2);
     printf("%s\r",s2);
  }
 }
}
#endif
