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

/* $Id$ */

#ifndef _DEMINEUR_H_
#define _DEMINEUR_H_

typedef struct
{
   int   mine ;     /* 1 if there's a mine, 0 otherwise */
   int   around ;   /* mine count around this square    */
   enum
   {
      HIDDEN    ,   /* hidden square    */
      FLAGGED   ,   /* flagged square   */
      QUESTION  ,   /* question mark    */
      UNCOVERED     /* uncovered square */
   }
   state ;          /* square state */
}
square_t ;

typedef struct
{
   int        rows ;      /* number of rows    */
   int        columns ;   /* number of columns */
   int        mines ;     /* number of mines   */
   square_t   **board ;   /* the game board    */
}
board_t ;

typedef enum
{
   PLAYING , WON , LOST
}
state_t ;

/* ------------------------------------------------------------------------- */

void demineur_initialize ( int option_mines ) ;

void demineur_start_timer ( ) ;

void demineur_stop_timer ( ) ;

void demineur_play ( int row , int column ) ;

int demineur_hidden ( int row , int column ) ;

int demineur_hidden_count ( int row , int column ) ;

int demineur_flags ( int row , int column ) ;

void demineur_clear ( int row , int column ) ;

void demineur_flag_question ( int row , int column ) ;

void demineur_end ( ) ;

#endif /* _DEMINEUR_H_ */
