/*
 * Copyright © 1993-1999 Marc Baudoin <babafou@babafou.eu.org>
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that copyright notice and this permission
 * notice appear in supporting documentation.  The author makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 */

static char *const cvsid = "$Id$" ;

#include <sys/types.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <time.h>

#include "demineur.h"
#include "xdemineur.h"

/* ------------------------------------------------------------------------- */

void sigalrm ( int sig ) ;

/* ------------------------------------------------------------------------- */

extern volatile time_t   elapsed ;
extern state_t           state ;

/* ------------------------------------------------------------------------- */

int main ( int argc , char **argv )
{
   int                i , option_mines = 0 ;
   char               *display_name = NULL , *geometry = NULL ;
   struct sigaction   act ;

   /* command line arguments */

   for ( i = 1 ; i < argc ; i ++ )
   {
      if ( strcmp ( argv[i] , "-v" ) == 0 )
      {
         unsigned char *message = "\
xdémineur 2.1.1\n\
\n\
Copyright © 1993-1999 Marc Baudoin <babafou@babafou.eu.org>\n\
\n\
Permission to use, copy, modify, and distribute this software and\n\
its documentation for any purpose and without fee is hereby\n\
granted, provided that the above copyright notice appear in all\n\
copies and that both that copyright notice and this permission\n\
notice appear in supporting documentation.  The author makes no\n\
representations about the suitability of this software for any\n\
purpose.  It is provided \"as is\" without express or implied\n\
warranty.\n" ;

         printf ( "%s" , message ) ;

         exit ( EX_OK ) ;
      }
      else if ( strcmp ( argv[i] , "-display" ) == 0 )
      {
         i ++ ;
         if ( i < argc )
         {
            display_name = argv[i] ;
            continue ;
         }
         i -- ;
      }
      else if ( strcmp ( argv[i] , "-geometry" ) == 0 )
      {
         i ++ ;
         if ( i < argc )
         {
            geometry = argv[i] ;
            continue ;
         }
         i -- ;
      }
      else if ( strcmp ( argv[i] , "-m" ) == 0 )
      {
         i ++ ;
         if ( i < argc )
         {
            option_mines = atoi ( argv[i] ) ;
            continue ;
         }
         i -- ;
      }

      fprintf ( stderr , "bad command line option \"%s\"\n" , argv[i] ) ;
      fprintf ( stderr ,
        "usage: %s [-v] [-display displayname] [-geometry geom] [-m mines]\n" ,
                argv[0] ) ;
      exit ( EX_USAGE ) ;
   }

   /* initialization */

   xdemineur_initialize ( argc , argv , display_name , geometry ) ;

   srand ( time ( NULL ) ) ;

   demineur_initialize ( option_mines ) ;

   act.sa_handler = sigalrm ;
   sigemptyset ( &act.sa_mask ) ;
#ifdef SA_RESTART
   act.sa_flags = SA_RESTART ;
#else
   act.sa_flags = 0 ;
#endif
   if ( sigaction ( SIGALRM , &act , NULL ) == -1 )
   {
      perror ( "sigaction" ) ;
      exit ( EX_OSERR ) ;
   }

   /* event loop */

   xdemineur_event_loop ( ) ;

   /* the end */

   demineur_end ( ) ;

   exit ( EX_OK ) ;
}

/* ------------------------------------------------------------------------- */

void sigalrm ( int sig )
{
   elapsed ++ ;
}
