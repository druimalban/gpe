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

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

/* ------------------------------------------------------------------------- */

void *xmalloc ( size_t size )
{
   void *ptr ;

   ptr = malloc ( size ) ;
   if ( ptr == NULL )
   {
      fprintf ( stderr , "Can't allocate memory.\n" ) ;
      exit ( EX_SOFTWARE ) ;
   }

   return ptr ;
}
