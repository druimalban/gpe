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

#include <sys/time.h>

#include "demineur.h"
#include "util.h"
#include "xdemineur.h"

/* ------------------------------------------------------------------------- */

typedef struct row_column
{
   int row ;
   int column ;
   struct row_column *next ;
}
row_column_t ;

/* ------------------------------------------------------------------------- */

int demineur_flag_count ( int row , int column ) ;

void demineur_won ( ) ;

/* ------------------------------------------------------------------------- */

board_t           board ;
int               mines ;
volatile time_t   elapsed ;
state_t           state ;

/* ------------------------------------------------------------------------- */

void demineur_initialize ( int option_mines )
{
   int            row , column , m , openings ;
   row_column_t   *rc = NULL , *ptr = NULL ;

   /* There are two extra rows and two extra columns so that
    * there's no need to handle side effects when counting the
    * number of surrounding mines.
    */

   board.board = ( square_t ** ) xmalloc ( ( board.rows + 2 )
                                           * sizeof ( square_t * ) ) ;
   for ( row = 0 ; row < board.rows + 2 ; row ++ )
   {
      board.board[row] = ( square_t * ) xmalloc ( ( board.columns + 2 )
                                                  * sizeof ( square_t ) ) ;
   }

   if ( option_mines != 0 )
   {
      if ( option_mines >= board.rows * board.columns )
      {
         printf ( "Too many mines, using default number.\n" ) ;
         mines = board.mines = board.rows * board.columns / 4.8 ;
      }
      else
      {
         mines = board.mines = option_mines ;
      }
   }
   else
   {
      mines = board.mines = board.rows * board.columns / 4.8 ;
   }

   elapsed = 0 ;
   state = PLAYING ;

   /* initialize board */

   for ( row = 0 ; row < board.rows + 2 ; row ++ )
   {
      for ( column = 0 ; column < board.columns + 2 ; column ++ )
      {
         board.board[row][column].mine   = 0 ;
         board.board[row][column].around = 0 ;
         board.board[row][column].state  = HIDDEN ;
      }
   }

   /* special treatment for extra squares */

   for ( row = 0 ; row < board.rows + 2 ; row ++ )
   {
      board.board[row][0].state                 = UNCOVERED ;
      board.board[row][board.columns + 1].state = UNCOVERED ;
   }

   for ( column = 0 ; column < board.columns + 2 ; column ++ )
   {
      board.board[0][column].state              = UNCOVERED ;
      board.board[board.rows + 1][column].state = UNCOVERED ;
   }

   /* mines deposit */

   for ( row = 1 ; row <= board.rows ; row ++ )
   {
      for ( column = 1 ; column <= board.columns ; column ++ )
      {
         if ( rc == NULL )   /* first square */
         {
            rc = ( row_column_t * ) xmalloc ( sizeof ( row_column_t ) ) ;
            ptr = rc ;
         }
         else
         {
            ptr->next = ( row_column_t * )
                        xmalloc ( sizeof ( row_column_t ) ) ;
            ptr = ptr->next ;
         }

         ptr->row    = row ;
         ptr->column = column ;
         ptr->next   = NULL ;
      }
   }

   for ( m = 0 ; m < board.mines ; m ++ )
   {
      int alea = rand ( ) % ( board.rows * board.columns - m ) ;

      if ( alea == 0 )
      {
         ptr = rc ;
         rc = rc->next ;
      }
      else
      {
         row_column_t *tmp ;

         for ( ptr = rc ; alea > 1 ; alea -- , ptr = ptr->next )
         {
            continue ;
         }

         tmp = ptr->next ;
         ptr->next = ptr->next->next ;
         ptr = tmp ;
      }

      board.board[ptr->row][ptr->column].mine = 1 ;
      free ( ptr ) ;
   }

   /* free memory used by squares without mine */

   for ( ptr = rc ; ptr != NULL ; )
   {
      row_column_t *tmp = ptr->next ;

      free ( ptr ) ;
      ptr = tmp ;
   }
   rc = NULL ;

   /* mines counting */

   for ( row = 1 ; row <= board.rows ; row ++ )
   {
      for ( column = 1 ; column <= board.columns ; column ++ )
      {
         board.board[row][column].around =
                                        board.board[row - 1][column - 1].mine +
                                        board.board[row - 1][column    ].mine +
                                        board.board[row - 1][column + 1].mine +
                                        board.board[row    ][column - 1].mine +
                                        board.board[row    ][column + 1].mine +
                                        board.board[row + 1][column - 1].mine +
                                        board.board[row + 1][column    ].mine +
                                        board.board[row + 1][column + 1].mine ;
      }
   }

   /* automatic opening */

   openings = 0 ;
   for ( row = 1 ; row <= board.rows ; row ++ )
   {
      for ( column = 1 ; column <= board.columns ; column ++ )
      {
         if ( board.board[row][column].around == 0
              && ! board.board[row][column].mine )
         {
            openings ++ ;

            if ( rc == NULL )   /* first square */
            {
               rc = ( row_column_t * ) xmalloc ( sizeof ( row_column_t ) ) ;
               ptr = rc ;
            }
            else
            {
               ptr->next = ( row_column_t * )
                           xmalloc ( sizeof ( row_column_t ) ) ;
               ptr = ptr->next ;
            }

            ptr->row    = row ;
            ptr->column = column ;
            ptr->next   = NULL ;
         }
      }
   }

   if ( openings != 0 )
   {
      int alea = rand ( ) % openings , i ;

      for ( i = 0 , ptr = rc ; i < alea ; i ++ , ptr = ptr->next )
      {
         continue ;
      }

      demineur_play ( ptr->row , ptr->column ) ;
   }

   for ( ptr = rc ; ptr != NULL ; )
   {
      row_column_t *tmp = ptr->next ;

      free ( ptr ) ;
      ptr = tmp ;
   }
}

/* ------------------------------------------------------------------------- */

void demineur_start_timer ( )
{
   struct itimerval itimerval ;

   itimerval.it_interval.tv_sec  = 1 ;
   itimerval.it_interval.tv_usec = 0 ;
   itimerval.it_value.tv_sec     = 1 ;
   itimerval.it_value.tv_usec    = 0 ;
   if ( setitimer ( ITIMER_REAL , &itimerval , NULL ) == -1 )
   {
      perror ( "setitimer" ) ;
      exit ( EX_OSERR ) ;
   }
}

/* ------------------------------------------------------------------------- */

void demineur_stop_timer ( )
{
   struct itimerval itimerval ;

   timerclear ( &itimerval.it_interval ) ;
   timerclear ( &itimerval.it_value    ) ;

   if ( setitimer ( ITIMER_REAL , &itimerval , NULL ) == -1 )
   {
      perror ( "setitimer" ) ;
      exit ( EX_OSERR ) ;
   }
}

/* ------------------------------------------------------------------------- */

void demineur_play ( int row , int column )
{
   if ( row < 1 || row > board.rows || column < 1 || column > board.columns )
   {
      return ;
   }

   if ( board.board[row][column].state == HIDDEN )
   {
      board.board[row][column].state = UNCOVERED ;
      xdemineur_square ( row , column ) ;
      if ( board.board[row][column].mine )
      {
         state = LOST ;
         demineur_stop_timer ( ) ;
         xdemineur_display ( ) ;
      }
      else if ( board.board[row][column].around == 0 )
      {
         demineur_clear ( row , column ) ;
      }
   }

   if ( mines == 0 )
   {
      demineur_won ( ) ;
   }
}

/* ------------------------------------------------------------------------- */

int demineur_hidden ( int row , int column )
{
   return demineur_hidden_count ( row - 1 , column - 1 ) +
          demineur_hidden_count ( row - 1 , column     ) +
          demineur_hidden_count ( row - 1 , column + 1 ) +
          demineur_hidden_count ( row     , column - 1 ) +
          demineur_hidden_count ( row     , column + 1 ) +
          demineur_hidden_count ( row + 1 , column - 1 ) +
          demineur_hidden_count ( row + 1 , column     ) +
          demineur_hidden_count ( row + 1 , column + 1 ) ;
}

/* ------------------------------------------------------------------------- */

int demineur_hidden_count ( int row , int column )
{
   return ( board.board[row][column].state == HIDDEN ) ? 1 : 0 ;
}

/* ------------------------------------------------------------------------- */

int demineur_flags ( int row , int column )
{
   return demineur_flag_count ( row - 1 , column - 1 ) +
          demineur_flag_count ( row - 1 , column     ) +
          demineur_flag_count ( row - 1 , column + 1 ) +
          demineur_flag_count ( row     , column - 1 ) +
          demineur_flag_count ( row     , column + 1 ) +
          demineur_flag_count ( row + 1 , column - 1 ) +
          demineur_flag_count ( row + 1 , column     ) +
          demineur_flag_count ( row + 1 , column + 1 ) ;
}

/* ------------------------------------------------------------------------- */

int demineur_flag_count ( int row , int column )
{
   return ( board.board[row][column].state == FLAGGED ) ? 1 : 0 ;
}

/* ------------------------------------------------------------------------- */

void demineur_clear ( int row , int column )
{
   demineur_play ( row - 1 , column - 1 ) ;
   demineur_play ( row - 1 , column     ) ;
   demineur_play ( row - 1 , column + 1 ) ;
   demineur_play ( row     , column - 1 ) ;
   demineur_play ( row     , column + 1 ) ;
   demineur_play ( row + 1 , column - 1 ) ;
   demineur_play ( row + 1 , column     ) ;
   demineur_play ( row + 1 , column + 1 ) ;
}

/* ------------------------------------------------------------------------- */

void demineur_flag_question ( int row , int column )
{
   switch ( board.board[row][column].state )
   {
   case HIDDEN :
      if ( mines > 0 )
      {
         board.board[row][column].state = FLAGGED ;
         mines -- ;
         if ( mines == 0 )
         {
            demineur_won ( ) ;
         }
      }
      break ;
   case FLAGGED :
      board.board[row][column].state = QUESTION ;
      mines ++ ;
      break ;
   case QUESTION :
      board.board[row][column].state = HIDDEN ;
      break ;
   case UNCOVERED :
      break ;
   }
}

/* ------------------------------------------------------------------------- */

void demineur_won ( )
{
   int row , column ;

   if (state == LOST) return ;
   /* [RT] this may happen in case no hidden or questioned cases are left
      but the last uncovered case was holdng a mine. This means that one of
      the flagged cases was in fact not holding a mine.
   */

   for ( row = 1 ; row <= board.rows ; row ++ )
   {
      for ( column = 1 ; column <= board.columns ; column ++ )
      {
         if ( board.board[row][column].state == HIDDEN ||
              board.board[row][column].state == QUESTION )
         {
            return ;
         }
      }
   }

   /* if you get there, it's that you've won */

   state = WON ;
   demineur_stop_timer ( ) ;
   xdemineur_face ( ) ;
}

/* ------------------------------------------------------------------------- */

void demineur_end ( )
{
   int row ;

   for ( row = 0 ; row < board.rows + 2 ; row ++ )
   {
      free ( board.board[row] ) ;
   }
   free ( board.board ) ;
}
