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

#ifndef _XDEMINEUR_H_
#define _XDEMINEUR_H_

#define ROWS_MIN      8
#define COLUMNS_MIN   15

#define EDGE              5   /* edge around the game board */
#define RELIEF_WIDTH       3   /* width of the relief */

#define INFO              25

#define SQUARE_WIDTH      10   /* width  of a square */
#define SQUARE_HEIGHT     10   /* height of a square */

#define BASE_WIDTH        ( EDGE + 1 + EDGE )
#define BASE_HEIGHT       ( EDGE + INFO + EDGE + 1 + EDGE )
#define WIDTH_INC         ( SQUARE_WIDTH  + 1 )
#define HEIGHT_INC        ( SQUARE_HEIGHT + 1 )

#define MIN_WIDTH  ( BASE_WIDTH  + COLUMNS_MIN * WIDTH_INC  )
#define MIN_HEIGHT ( BASE_HEIGHT + ROWS_MIN    * HEIGHT_INC )

#define X_BOARD   EDGE
#define Y_BOARD   ( EDGE + INFO + EDGE )

#define FACE_WIDTH  20
#define FACE_HEIGHT 20
#define Y_FACE      ( ( Y_BOARD - FACE_HEIGHT ) / 2 )

#define DIGIT_WIDTH  10
#define DIGIT_HEIGHT 15

#define X_DIGITS        ( ( Y_BOARD - DIGIT_WIDTH  ) / 2 )
#define Y_DIGITS        ( ( Y_BOARD - DIGIT_HEIGHT ) / 2 )

/* ------------------------------------------------------------------------- */

void xdemineur_initialize ( int argc , char **argv ,
                            char *display_name , char *geometry ) ;

void xdemineur_event_loop ( ) ;

void xdemineur_display ( ) ;

void xdemineur_face ( ) ;

void xdemineur_timer ( ) ;

void xdemineur_square ( int row , int column ) ;

#endif /* _XDEMINEUR_H_ */
