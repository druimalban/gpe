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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include <sys/time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <X11/xpm.h>

#include "demineur.h"
#include "xdemineur.h"

#include "xdemineur.xpm"

#include "pixmaps/face_normal.xpm"
#include "pixmaps/face_click.xpm"
#include "pixmaps/face_play.xpm"
#include "pixmaps/face_happy.xpm"
#include "pixmaps/face_sad.xpm"

#include "pixmaps/digit_0.xpm"
#include "pixmaps/digit_1.xpm"
#include "pixmaps/digit_2.xpm"
#include "pixmaps/digit_3.xpm"
#include "pixmaps/digit_4.xpm"
#include "pixmaps/digit_5.xpm"
#include "pixmaps/digit_6.xpm"
#include "pixmaps/digit_7.xpm"
#include "pixmaps/digit_8.xpm"
#include "pixmaps/digit_9.xpm"

#include "pixmaps/square_0.xpm"
#include "pixmaps/square_1.xpm"
#include "pixmaps/square_2.xpm"
#include "pixmaps/square_3.xpm"
#include "pixmaps/square_4.xpm"
#include "pixmaps/square_5.xpm"
#include "pixmaps/square_6.xpm"
#include "pixmaps/square_7.xpm"
#include "pixmaps/square_8.xpm"

#include "pixmaps/relief.xpm"
#include "pixmaps/flag.xpm"
#include "pixmaps/question.xpm"
#include "pixmaps/mine.xpm"
#include "pixmaps/mine_lost.xpm"
#include "pixmaps/mine_false.xpm"

/* ------------------------------------------------------------------------- */

typedef enum
{
   ITEM_NOTHING , ITEM_SQUARE , ITEM_FACE
}
item_t ;

typedef struct
{
   int row , column ;
   enum
   {
      STATE_NONE , STATE_UNCOVER , STATE_CLEAR , STATE_FLAG_QUESTION ,
      STATE_FACE
   }
   state ;
}
previous_t ;

typedef enum
{
   RAISED , SUNKEN
}
relief_t ;

typedef enum
{
   INSIDE , OUTSIDE
}
inout_t ;

/* ------------------------------------------------------------------------- */

void xdemineur_colors ( ) ;

void xdemineur_pixmaps ( ) ;

void xdemineur_xpm ( char **data , Pixmap *pixmap_return ) ;

item_t mouse ( int x , int y , int *row , int *column ) ;


void xdemineur_frames ( ) ;

void xdemineur_frame ( int x1 , int y1 , int x2 , int y2 , int width ,
                       relief_t relief , inout_t inoutside ) ;

void xdemineur_face_click ( ) ;

void xdemineur_face_play ( ) ;

void xdemineur_face_display ( Pixmap face ) ;

void xdemineur_mines ( ) ;

void xdemineur_digits ( int number , int digits , int x , int y ) ;

void xdemineur_grid ( ) ;

void xdemineur_square_play ( int row , int column ) ;

void xdemineur_squares_clear ( int row , int column ) ;

void xdemineur_squares ( int row , int column ) ;

void xdemineur_end ( ) ;

/* ------------------------------------------------------------------------- */

extern board_t           board ;
extern int               mines ;
extern volatile time_t   elapsed ;
extern state_t           state ;

static Display   *display ;
static int       screen ;
static Window    window ;
static Atom      protocol[1] ;
static GC        gc ;
static Pixmap	 icon_pixmap, icon_mask;

static unsigned long black , white , gray , light_gray ;

static Pixmap face_normal , face_click , face_play , face_happy , face_sad ,
              digit[10] , square[9] ,
              relief , flag , question , mine , mine_lost , mine_false ;

/* ------------------------------------------------------------------------- */

void xdemineur_initialize ( int argc , char **argv ,
                            char *display_name , char *geometry )
{
   XSizeHints      size_hints ;
   int             x_pos = 0 , y_pos = 0 ,
                   width = MIN_WIDTH , height = MIN_HEIGHT ;
   char            *window_title = "xdémineur" , *icon_title = "xdémineur" ;
   char		   *utf8_title = "xdÃ©mineur";
   XTextProperty   window_name , icon_name ;
   XWMHints        wm_hints ;
   XClassHint      class_hints ;
   XGCValues       values ;

   display = XOpenDisplay ( display_name ) ;
   if ( display == NULL )
   {
      fprintf ( stderr , "Error: Can't open display: %s\n" ,
                XDisplayName ( display_name ) ) ;
      exit ( EX_OSERR ) ;
   }

   screen = DefaultScreen ( display ) ;

   board.columns = COLUMNS_MIN ;
   board.rows    = ROWS_MIN ;

   size_hints.flags = 0 ;
   if ( geometry != NULL )
   {
      int            flags , x , y ;
      unsigned int   w , h ;

      flags = XParseGeometry ( geometry , &x , &y , &w , &h ) ;

      if ( WidthValue & flags )
      {
	 w -= BASE_WIDTH;
	 w /= WIDTH_INC;
         if ( w < COLUMNS_MIN )
         {
            board.columns = COLUMNS_MIN ;
            fprintf ( stderr ,
                      "%d columns is too small!  Using %d columns instead.\n" ,
                      w , COLUMNS_MIN ) ;
         }
         else
         {
            board.columns = w ;
            width  = BASE_WIDTH + board.columns * WIDTH_INC ;
         }
      }

      if ( HeightValue & flags )
      {
	 h -= BASE_HEIGHT;
	 h /= HEIGHT_INC;
         if ( h < ROWS_MIN )
         {
            board.rows = ROWS_MIN ;
            fprintf ( stderr ,
                      "%d rows is too small!  Using %d rows instead.\n" ,
                      h , ROWS_MIN ) ;
         }
         else
         {
            board.rows = h ;
            height = BASE_HEIGHT + board.rows * HEIGHT_INC ;
         }
      }

      if ( XValue & flags )
      {
         size_hints.flags = USPosition ;
         if ( XNegative & flags )
         {
            x_pos = DisplayWidth ( display , screen ) - width + x ;
         }
         else
         {
            x_pos = x ;
         }
      }

      if ( YValue & flags )
      {
         size_hints.flags = USPosition ;
         if ( YNegative & flags )
         {
            y_pos = DisplayHeight ( display , screen ) - height + y ;
         }
         else
         {
            y_pos = y ;
         }
      }
   }

   window = XCreateSimpleWindow ( display , RootWindow ( display , screen ) ,
                                  x_pos , y_pos , width , height , 1 ,
                                  BlackPixel ( display , screen ) ,
                                  WhitePixel ( display , screen ) ) ;

   XStringListToTextProperty ( &window_title , 1 , &window_name ) ;
   XStringListToTextProperty ( &icon_title   , 1 , &icon_name   ) ;

   size_hints.flags      |= USSize | PMinSize | PMaxSize |
                            PResizeInc | PBaseSize ;
   size_hints.min_width   = size_hints.max_width  = width ;
   size_hints.min_height  = size_hints.max_height = height ;
   size_hints.width_inc   = WIDTH_INC ;
   size_hints.height_inc  = HEIGHT_INC ;
   size_hints.base_width  = BASE_WIDTH ;
   size_hints.base_height = BASE_HEIGHT ;

   XpmCreatePixmapFromData ( display ,
			     RootWindow ( display , screen ) ,
			     xdemineur_bits ,
			     &icon_pixmap, &icon_mask, NULL);
   
   wm_hints.flags         = InputHint | StateHint | IconPixmapHint | IconMaskHint | WindowGroupHint;
   wm_hints.input         = True ;
   wm_hints.initial_state = NormalState ;
   wm_hints.icon_pixmap   = icon_pixmap ;
   wm_hints.icon_mask     = icon_mask ;
   wm_hints.window_group  = window ;

   class_hints.res_name  = argv[0] ;
   class_hints.res_class = "XDémineur" ;

   XSetWMProperties ( display , window ,
                      &window_name , &icon_name ,
                      argv , argc ,
                      &size_hints , &wm_hints , &class_hints ) ;

   XSetCommand ( display , window , argv , argc ) ;

   XChangeProperty (display, window,
		    XInternAtom (display, "_NET_WM_NAME", False),
		    XInternAtom (display, "UTF8_STRING", False),
		    8, PropModeReplace, utf8_title, strlen (utf8_title));

   XChangeProperty (display, window,
		    XInternAtom (display, "_NET_WM_ICON_NAME", False),
		    XInternAtom (display, "UTF8_STRING", False),
		    8, PropModeReplace, utf8_title, strlen (utf8_title));

   protocol[0] = XInternAtom ( display , "WM_DELETE_WINDOW" , False ) ;
   XSetWMProtocols ( display , window , protocol , 1 ) ;

   values.foreground         = BlackPixel ( display , screen ) ;
   values.background         = WhitePixel ( display , screen ) ;
   values.graphics_exposures = False ;
   gc = XCreateGC ( display , window ,
                    GCForeground | GCBackground | GCGraphicsExposures ,
                    &values ) ;

   XSelectInput ( display , window ,
                  KeyPressMask         |
                  ButtonPressMask      |
                  ButtonReleaseMask    |
                  ExposureMask         |
                  VisibilityChangeMask |
                  StructureNotifyMask ) ;

   xdemineur_colors ( ) ;
   xdemineur_pixmaps ( ) ;

   XMapWindow ( display , window ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_colors ( )
{
   Colormap   default_colormap ;
   XColor     color ;

   default_colormap = DefaultColormap ( display , screen ) ;
   XParseColor ( display , default_colormap , "black" , &color ) ;
   XAllocColor ( display , default_colormap , &color ) ;
   black = color.pixel ;
   XParseColor ( display , default_colormap , "white" , &color ) ;
   XAllocColor ( display , default_colormap , &color ) ;
   white = color.pixel ;
   XParseColor ( display , default_colormap , "gray50" , &color ) ;
   XAllocColor ( display , default_colormap , &color ) ;
   gray = color.pixel ;
   XParseColor ( display , default_colormap , "gray70" , &color ) ;
   XAllocColor ( display , default_colormap , &color ) ;
   light_gray = color.pixel ;

   XSetWindowBackground ( display , window , light_gray ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_pixmaps ( )
{
   xdemineur_xpm ( xpm_face_normal , &face_normal ) ;
   xdemineur_xpm ( xpm_face_click  , &face_click  ) ;
   xdemineur_xpm ( xpm_face_play   , &face_play   ) ;
   xdemineur_xpm ( xpm_face_happy  , &face_happy  ) ;
   xdemineur_xpm ( xpm_face_sad    , &face_sad    ) ;

   xdemineur_xpm ( xpm_digit_0 , &digit[0] ) ;
   xdemineur_xpm ( xpm_digit_1 , &digit[1] ) ;
   xdemineur_xpm ( xpm_digit_2 , &digit[2] ) ;
   xdemineur_xpm ( xpm_digit_3 , &digit[3] ) ;
   xdemineur_xpm ( xpm_digit_4 , &digit[4] ) ;
   xdemineur_xpm ( xpm_digit_5 , &digit[5] ) ;
   xdemineur_xpm ( xpm_digit_6 , &digit[6] ) ;
   xdemineur_xpm ( xpm_digit_7 , &digit[7] ) ;
   xdemineur_xpm ( xpm_digit_8 , &digit[8] ) ;
   xdemineur_xpm ( xpm_digit_9 , &digit[9] ) ;

   xdemineur_xpm ( xpm_square_0 , &square[0] ) ;
   xdemineur_xpm ( xpm_square_1 , &square[1] ) ;
   xdemineur_xpm ( xpm_square_2 , &square[2] ) ;
   xdemineur_xpm ( xpm_square_3 , &square[3] ) ;
   xdemineur_xpm ( xpm_square_4 , &square[4] ) ;
   xdemineur_xpm ( xpm_square_5 , &square[5] ) ;
   xdemineur_xpm ( xpm_square_6 , &square[6] ) ;
   xdemineur_xpm ( xpm_square_7 , &square[7] ) ;
   xdemineur_xpm ( xpm_square_8 , &square[8] ) ;

   xdemineur_xpm ( xpm_relief     , &relief ) ;
   xdemineur_xpm ( xpm_flag       , &flag ) ;
   xdemineur_xpm ( xpm_question   , &question ) ;
   xdemineur_xpm ( xpm_mine       , &mine ) ;
   xdemineur_xpm ( xpm_mine_lost  , &mine_lost ) ;
   xdemineur_xpm ( xpm_mine_false , &mine_false ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_xpm ( char **data , Pixmap *pixmap_return )
{
   int xpm_status ;

   xpm_status = XpmCreatePixmapFromData ( display , window ,
                                          data , pixmap_return ,
                                          NULL , NULL ) ;
   if ( xpm_status != XpmSuccess )
   {
      fprintf ( stderr ,
                "XpmError: %s\n" , XpmGetErrorString ( xpm_status ) ) ;
      exit ( EX_OSERR ) ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_event_loop ( )
{
   fd_set       readfds ;
   XEvent       event ;
   Region       region ;
   XRectangle   rectangle ;
   item_t       item ;
   previous_t   previous ;

   FD_ZERO ( &readfds ) ;
   region = XCreateRegion ( ) ;
   for ( ; ; )
   {
      int row , column ;

      if ( XPending ( display ) == 0 )   /* no more events to handle */
      {
         FD_SET ( ConnectionNumber ( display ) , &readfds ) ;
         if ( select ( ConnectionNumber ( display ) + 1 , &readfds , NULL ,
                       NULL , NULL ) == -1 )   /* wait for events or signal */
         {
            if ( errno == EINTR )   /* interrupted by signal */
            {
               xdemineur_timer ( ) ;
               XFlush ( display ) ;
               continue ;
            }
         }
         /* an event occurred, proceed to XNextEvent() */
      }

      XNextEvent ( display , &event ) ;
      switch ( event.type )
      {
      case ConfigureNotify:
	 {
	   int height = event.xconfigure.height, width = event.xconfigure.width;
	   int changed = 0;
	   height -= BASE_HEIGHT;
	   width -= BASE_WIDTH;
	   height /= HEIGHT_INC;
	   width /= WIDTH_INC;
	   if (height >= ROWS_MIN)
	     {
	       if (board.rows != height)
		 {
		   changed = 1;
		   board.rows = height;
		 }
	     }
	   if (width >= COLUMNS_MIN)
	     {
	       if (board.columns != width)
		 {
		   board.columns = width;
		   changed = 1;
		 }
	     }

	   if (changed)
	     demineur_initialize ( 0 ) ;
	 }
	 break;
      case KeyPress :
         switch ( XLookupKeysym ( &event.xkey , event.xkey.state ) )
         {
         case XK_Escape :
         case XK_Q :
         case XK_q :
            XDestroyRegion ( region ) ;
            xdemineur_end ( ) ;
            return ;
         }
         break ;
      case ButtonPress :
         switch ( mouse ( event.xbutton.x , event.xbutton.y ,
                          &row , &column ) )
         {
         case ITEM_SQUARE :
            if ( state != PLAYING )
            {
               previous.state = STATE_NONE ;
               break ;
            }

            switch ( event.xbutton.button )
            {
            case Button1 :   /* uncover square */
               if ( board.board[row][column].state == HIDDEN )
               {
                  xdemineur_face_play ( ) ;
                  xdemineur_square_play ( row , column ) ;
                  previous.row    = row ;
                  previous.column = column ;
                  previous.state  = STATE_UNCOVER ;
               }
               else
               {
                  previous.state = STATE_NONE ;
               }
               break ;
            case Button2 :   /* uncover squares around */
               if ( board.board[row][column].state == UNCOVERED )
               {
                  if ( demineur_hidden ( row , column ) != 0 &&
                       board.board[row][column].around
                       == demineur_flags ( row , column ) )
                  {
                     xdemineur_face_play ( ) ;
                     xdemineur_squares_clear ( row , column ) ;
                     previous.row    = row ;
                     previous.column = column ;
                     previous.state  = STATE_CLEAR ;
                  }
                  else
                  {
                     previous.state = STATE_NONE ;
                  }
               }
               break ;
            case Button3 :   /* put flag or question mark */
               if ( board.board[row][column].state == UNCOVERED )
               {
                  previous.state = STATE_NONE;
                  break ;
               }

               xdemineur_face_play ( ) ;
               demineur_flag_question ( row , column ) ;
               xdemineur_mines ( ) ;
               xdemineur_square ( row , column ) ;
               previous.state = STATE_FLAG_QUESTION ;
               break ;
            }
            break ;
         case ITEM_FACE :
            xdemineur_face_click ( ) ;
            previous.state = STATE_FACE ;
            break ;
         case ITEM_NOTHING :
            previous.state = STATE_NONE ;
            break ;
         }
         break ;
      case ButtonRelease :
         item = mouse ( event.xbutton.x , event.xbutton.y , &row , &column ) ;
         switch ( previous.state )
         {
         case STATE_NONE :
            break ;
         case STATE_UNCOVER :
            if ( row == previous.row && column == previous.column )
            {
               demineur_play ( row , column ) ;
            }
            else
            {
               xdemineur_square ( previous.row , previous.column ) ;
            }
            xdemineur_face ( ) ;
            break ;
         case STATE_CLEAR :
            if ( row == previous.row && column == previous.column )
            {
               demineur_clear ( row , column ) ;
            }
            else
            {
               xdemineur_squares ( previous.row , previous.column ) ;
            }
            xdemineur_face ( ) ;
            break ;
         case STATE_FLAG_QUESTION :
            xdemineur_face ( ) ;
            break ;
         case STATE_FACE :
            if ( item == ITEM_FACE )   /* new game */
            {
               demineur_end ( ) ;
               demineur_initialize ( 0 ) ;
               xdemineur_display ( ) ;
               demineur_start_timer ( ) ;
            }
            else
            {
               xdemineur_face ( ) ;
            }
            break ;
         }
         previous.state = STATE_NONE ;
         break ;
      case Expose :
         rectangle.x      = ( short )          event.xexpose.x ;
         rectangle.y      = ( short )          event.xexpose.y ;
         rectangle.width  = ( unsigned short ) event.xexpose.width ;
         rectangle.height = ( unsigned short ) event.xexpose.height ;
         XUnionRectWithRegion ( &rectangle , region , region ) ;
         if ( event.xexpose.count == 0 )
         {
            XSetRegion ( display , gc , region ) ;
            xdemineur_display ( ) ;
            XSetClipMask ( display , gc , None ) ;
            XDestroyRegion ( region ) ;
            region = XCreateRegion ( ) ;
         }
         break ;
      case VisibilityNotify :
         switch ( event.xvisibility.state )
         {
         case VisibilityUnobscured :
            if ( state == PLAYING )
            {
               demineur_start_timer ( ) ;
            }
            break ;
         case VisibilityPartiallyObscured :
            break ;
         case VisibilityFullyObscured :
            if ( state == PLAYING )
            {
               demineur_stop_timer ( ) ;
            }
            break ;
         }
         break ;
      case UnmapNotify :   /* the window has been iconified */
         if ( state == PLAYING )
         {
            demineur_stop_timer ( ) ;
         }
         break ;
      case MapNotify :   /* the window has been deiconified */
         if ( state == PLAYING )
         {
            demineur_start_timer ( ) ;
         }
         break ;
      case ClientMessage :
         if ( event.xclient.data.l[0] == protocol[0] )
         {
            XDestroyRegion ( region ) ;
            xdemineur_end ( ) ;
            return ;
         }
         break ;
      }
   }
}

/* ------------------------------------------------------------------------- */

item_t mouse ( int x , int y , int *row , int *column )
{
   int board_width  = board.columns * WIDTH_INC  ,
       board_height = board.rows    * HEIGHT_INC ,
       x_face = ( BASE_WIDTH + board.columns * WIDTH_INC - FACE_WIDTH ) / 2 ;

   *row = *column = 0 ;

   if ( x > X_BOARD               &&
        x < X_BOARD + board_width &&
        y > Y_BOARD               &&
        y < Y_BOARD + board_height )
   {
      if ( ( x - X_BOARD ) % WIDTH_INC  == 0 ||
           ( y - Y_BOARD ) % HEIGHT_INC == 0 )
      {
         return ITEM_NOTHING ;
      }
      else
      {
         *column = 1 + ( x - X_BOARD ) / WIDTH_INC ;
         *row    = 1 + ( y - Y_BOARD ) / HEIGHT_INC ;
         return ITEM_SQUARE ;
      }
   }
   else if ( x >= x_face              &&
             x <= x_face + FACE_WIDTH &&
             y >= Y_FACE              &&
             y <= Y_FACE + FACE_HEIGHT )
   {
      return ITEM_FACE ;
   }
   else
   {
      return ITEM_NOTHING ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_display ( )
{
   int row , column ;

   xdemineur_frames ( ) ;
   xdemineur_face ( ) ;
   xdemineur_mines ( ) ;
   xdemineur_timer ( ) ;
   xdemineur_grid ( ) ;
   for ( row = 1 ; row <= board.rows ; row ++ )
   {
      for ( column = 1 ; column <= board.columns ; column ++ )
      {
         xdemineur_square ( row , column ) ;
      }
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_frames ( )
{
   int board_width   = board.columns * WIDTH_INC ;
   int board_height  = board.rows    * HEIGHT_INC ;
   int window_width  = BASE_WIDTH  + board_width ;
   int window_height = BASE_HEIGHT + board_height ;

   xdemineur_frame ( 0 , 0 , window_width - 1 , window_height - 1 ,
                     RELIEF_WIDTH , RAISED , INSIDE) ;
   xdemineur_frame ( X_BOARD , Y_BOARD ,
                     X_BOARD + board_width , Y_BOARD + board_height ,
                     RELIEF_WIDTH , SUNKEN , OUTSIDE) ;
   xdemineur_frame ( EDGE , EDGE , window_width - 1 - EDGE , Y_BOARD - EDGE ,
                     RELIEF_WIDTH , SUNKEN , OUTSIDE ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_frame ( int x1 , int y1 , int x2 , int y2 , int width ,
                       relief_t relief , inout_t inoutside )
{
   int coord ;

   if ( inoutside == OUTSIDE )
   {
      x1 -= width ; x2 += width ;
      y1 -= width ; y2 += width ;
   }

   XSetForeground ( display , gc , ( relief == RAISED ) ? white : gray ) ;
   for ( coord = 0 ; coord < width ; coord ++ )
   {
      XDrawLine ( display , window , gc ,
                  x1 , y1 + coord , x2 - coord , y1 + coord ) ;
      XDrawLine ( display , window , gc ,
                  x1 + coord , y1 , x1 + coord , y2 - coord ) ;
   }
   XSetForeground ( display , gc , ( relief == RAISED ) ? gray : white ) ;
   for ( coord = 0 ; coord < width ; coord ++ )
   {
      XDrawLine ( display , window , gc ,
                  x1 + 1 + coord , y2 - coord , x2 , y2 - coord ) ;
      XDrawLine ( display , window , gc ,
                  x2 - coord , y1 + 1 + coord , x2 - coord , y2 ) ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_face ( )
{
   switch ( state )
   {
   case PLAYING :
      xdemineur_face_display ( face_normal ) ;
      break ;
   case WON :
      xdemineur_face_display ( face_happy ) ;
      break ;
   case LOST :
      xdemineur_face_display ( face_sad ) ;
      break ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_face_click ( )
{
   xdemineur_face_display ( face_click ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_face_play ( )
{
   xdemineur_face_display ( face_play ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_face_display ( Pixmap face )
{
   XCopyArea ( display , face , window , gc ,
               0 , 0 , FACE_WIDTH , FACE_HEIGHT ,
               ( BASE_WIDTH + board.columns * WIDTH_INC - FACE_WIDTH ) / 2 ,
               Y_FACE ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_mines ( )
{
   xdemineur_digits ( mines , 4 , X_DIGITS , Y_DIGITS ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_timer ( )
{
   int x = BASE_WIDTH + board.columns * WIDTH_INC
           - X_DIGITS - 4 * DIGIT_WIDTH ;

   xdemineur_digits ( elapsed , 4 , x , Y_DIGITS ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_digits ( int number , int digits , int x , int y )
{
   int i , remainder = number ;

   for ( i = digits - 1 ; i >= 0 ; i -- , remainder /= 10 )
   {
      XCopyArea ( display , digit[remainder % 10] , window , gc ,
                  0 , 0 , DIGIT_WIDTH , DIGIT_HEIGHT ,
                  x + i * DIGIT_WIDTH , y ) ;
   }

   xdemineur_frame ( x , y ,
                     x + digits * DIGIT_WIDTH - 1 , y + DIGIT_HEIGHT - 1 ,
                     1 , SUNKEN , OUTSIDE ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_grid ( )
{
   int coord ,
       board_width  = board.columns * WIDTH_INC  + 1 ,
       board_height = board.rows    * HEIGHT_INC + 1 ;

   XSetForeground ( display , gc , black ) ;

   for ( coord = X_BOARD ;
         coord < X_BOARD + board_width ;
         coord += WIDTH_INC )
   {
      XDrawLine ( display , window , gc ,
                  coord , Y_BOARD , coord , Y_BOARD + board_height - 1 ) ;
   }

   for ( coord = Y_BOARD ;
         coord < Y_BOARD + board_height ;
         coord += HEIGHT_INC )
   {
      XDrawLine ( display , window , gc ,
                  X_BOARD , coord , X_BOARD + board_width - 1 , coord ) ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_square ( int row , int column )
{
   int x = X_BOARD + 1 + ( column - 1 ) * ( SQUARE_WIDTH  + 1 ) ,
       y = Y_BOARD + 1 + ( row    - 1 ) * ( SQUARE_HEIGHT + 1 ) ;

   if ( row < 1 || row > board.rows || column < 1 || column > board.columns )
   {
      return ;
   }

   switch ( board.board[row][column].state )
   {
      case HIDDEN :
         if ( state == LOST && board.board[row][column].mine )
         {
            XCopyArea ( display , mine , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         else
         {
            XCopyArea ( display , relief , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         break ;
      case FLAGGED :
         if ( state == LOST && ! board.board[row][column].mine )
         {
            XCopyArea ( display , mine_false , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         else
         {
            XCopyArea ( display , flag , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         break ;
      case QUESTION :
         if ( state == LOST && board.board[row][column].mine )
         {
            XCopyArea ( display , mine , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         else
         {
            XCopyArea ( display , question , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         break ;
      case UNCOVERED :
         if ( ! board.board[row][column].mine )
         {
            XCopyArea ( display , square[board.board[row][column].around] ,
                        window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         else
         {
            XCopyArea ( display , mine_lost , window , gc ,
                        0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
         }
         break ;
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_square_play ( int row , int column )
{
   int x = X_BOARD + 1 + ( column - 1 ) * ( SQUARE_WIDTH  + 1 ) ,
       y = Y_BOARD + 1 + ( row    - 1 ) * ( SQUARE_HEIGHT + 1 ) ;

   if ( row < 1 || row > board.rows || column < 1 || column > board.columns )
   {
      return ;
   }

   XCopyArea ( display , square[0] , window , gc ,
               0 , 0 , SQUARE_WIDTH , SQUARE_HEIGHT , x , y ) ;
}

/* ------------------------------------------------------------------------- */

void xdemineur_squares_clear ( int row , int column )
{
   int r , c ;

   for ( r = row - 1 ; r <= row + 1 ; r ++ )
   {
      for ( c = column - 1 ; c <= column + 1 ; c ++ )
      {
         if ( board.board[r][c].state == HIDDEN )
         {
            xdemineur_square_play ( r , c ) ;
         }
      }
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_squares ( int row , int column )
{
   int r , c ;

   for ( r = row - 1 ; r <= row + 1 ; r ++ )
   {
      for ( c = column - 1 ; c <= column + 1 ; c ++ )
      {
         xdemineur_square ( r , c ) ;
      }
   }
}

/* ------------------------------------------------------------------------- */

void xdemineur_end ( )
{
   int i ;

   XFreePixmap ( display , face_normal ) ;
   XFreePixmap ( display , face_click  ) ;
   XFreePixmap ( display , face_play   ) ;
   XFreePixmap ( display , face_happy  ) ;
   XFreePixmap ( display , face_sad    ) ;
   for ( i = 0 ; i <= 9 ; i ++ )
   {
      XFreePixmap ( display , digit[i] ) ;
   }
   for ( i = 0 ; i <= 8 ; i ++ )
   {
      XFreePixmap ( display , square[i] ) ;
   }
   XFreePixmap ( display , relief     ) ;
   XFreePixmap ( display , flag       ) ;
   XFreePixmap ( display , question   ) ;
   XFreePixmap ( display , mine       ) ;
   XFreePixmap ( display , mine_lost  ) ;
   XFreePixmap ( display , mine_false ) ;

   XFreeGC ( display , gc ) ;
   XFreePixmap ( display , icon_pixmap ) ;
   XFreePixmap ( display , icon_mask ) ;
   XDestroyWindow ( display , window ) ;
   XCloseDisplay ( display ) ;
}
