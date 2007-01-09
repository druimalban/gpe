/*
 * ol-irc - A small irc client using GTK+
 *
 * Copyright (C) 1998, 1999 Yann Grossel [Olrick]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "olirc.h"
#include "icons.h"

GdkPixmap *console_xpm = NULL;
GdkBitmap *console_xpm_mask = NULL;

GdkPixmap *channel_xpm = NULL;
GdkBitmap *channel_xpm_mask = NULL;

GdkPixmap *query_xpm = NULL;
GdkBitmap *query_xpm_mask = NULL;

GdkPixmap *server_xpm = NULL;
GdkBitmap *server_xpm_mask = NULL;

GdkPixmap *dcc_chat_xpm = NULL;
GdkBitmap *dcc_chat_xpm_mask = NULL;


char *Server_xpm[] = {
" 24 24 6 1",
"  c None",
"d c #FFFFFF",
". c #000000",
"a c #c3c3c3",
"b c #828282",
"c c #8888BB",
"                        ",
"                        ",
"                        ",
"     .............      ",
"    ..aaaaaaaaaaab.     ",
"    .dddddddddddbb.     ",
"    .d.........abb.     ",
"    .d.ccccccccabb.     ",
"    .d.ccccccccabb.     ",
"    .d.ccccccccabb.     ",
"    .d.ccccccccabb.     ",
"    .d.ccccccccabb.     ",
"    .d.ccccccccabb.     ",
"    .ddddddddddabb...   ",
"   ..bbbbbbbbbbbb.bdb.  ",
"  .a.............bdbb.  ",
" .ddddddddddddddddabb.  ",
" .daaaaaaaaaaaaaaabbb.  ",
" .daaaaaaaab....babb.   ",
" .bbbbbbbbbbbbbbbba.    ",
"  .................     ",
"                        ",
"                        ",
"                        "
};

char *prefs_xpm[] = {
"24 24 3 1",
" 	c None",
".	c #000000000000",
"X	c #FFFFFFFF0000",
"                        ",
"                        ",
"                        ",
"                        ",
"                        ",
"    ......              ",
"   .XXXXXX.             ",
"   .XXXXXX.             ",
"   ................     ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"   .XXXXXXXXXXXXXXX.    ",
"    ...............     ",
"                        ",
"                        ",
"                        ",
"                        ",
"                        ",
"                        "};

char *console_xpm_data[] = {
"16 16 17 1",
" 	c None",
".	c #66AE66",
"+	c #009000",
"@	c #60609B",
"#	c #8787B4",
"$	c #AEAECD",
"%	c #E66666",
"&	c #212176",
"*	c #CACADC",
"=	c #66AEAE",
"-	c #FF0000",
";	c #4B4B8C",
">	c #9292BC",
",	c #FFFFFF",
"'	c #009090",
")	c #B666B6",
"!	c #A000A0",
"       ..       ",
"      .++.      ",
"     .++++.     ",
"                ",
"      @@#$      ",
"  %  &@#$*$  =  ",
" %- ;&@>*,*$ '= ",
"%-- &&@#$*$# ''=",
"%-- &&;@#>#@ ''=",
" %- ;&&;@@@@ '= ",
"  %  &&&&&&  =  ",
"      ;&&;      ",
"                ",
"     )!!!!)     ",
"      )!!)      ",
"       ))       "};

char *channel_xpm_data[] = {
"16 16 16 1",
" 	c None",
".	c #2020C0",
"+	c #4040C0",
"@	c #6060C0",
"#	c #1010C0",
"$	c #7070C0",
"%	c #9090C0",
"&	c #A0A0C0",
"*	c #0000C0",
"=	c #5050C0",
"-	c #8080C0",
";	c #C0C0C0",
">	c #B0B0C0",
",	c #D0D0D0",
"'	c #E0E0E0",
")	c #3030C0",
"                ",
"  .+@@    .+@@  ",
" #+$%&&  #+$%&& ",
"*#=-;>=+@=+-;,;-",
"*.=-&#+$%&%$;',%",
"*#+$).=->';-&>&-",
"**)=*#=%>';%$-$=",
" *#+@=+$&%=+@=+ ",
" #+@%&%=@#+@%&% ",
"*#=-;,;@#.=%>';-",
"*.=-;',%*#=-;',%",
"*#+$&>&-*#+$&>&-",
"**)=$-$=**)=$-$=",
" **.)++  **.)++ ",
"  **##    **##  ",
"                "};

char * dcc_chat_xpm_data[] = {
"16 16 16 1",
" 	c None",
".	c #0000C0",
"+	c #1010C0",
"@	c #2020C0",
"#	c #3030C0",
"$	c #4040C0",
"%	c #5050C0",
"&	c #6060C0",
"*	c #7070C0",
"=	c #8080C0",
"-	c #9090C0",
";	c #A0A0C0",
">	c #C0C0C0",
",	c #B0B0C0",
"'	c #E0E0E0",
")	c #D0D0D0",
".++@##$%%&&**&%%",
"                ",
"         #$&&   ",
"       .@$&=--* ",
"   @$&&+#&=;>>; ",
" .@$&=--%&-,'',-",
" +#&=;>,;%-,'',-",
".+#&-,''>=*;,,;=",
".+#&-,')>-&*--*&",
"..#%*;,,;*$%&&% ",
"..@$&=-==$@@### ",
" ..@$%&&%+..+   ",
" ...+###@       ",
"   ...+         ",
"                ",
".++@##$%%&**&&%%"};


char *query_xpm_data[] = {
"16 16 16 1",
" 	c None",
".	c #2020C0",
"+	c #4040C0",
"@	c #6060C0",
"#	c #0000C0",
"$	c #8080C0",
"%	c #9090C0",
"&	c #7070C0",
"*	c #1010C0",
"=	c #3030C0",
"-	c #A0A0C0",
";	c #C0C0C0",
">	c #5050C0",
",	c #B0B0C0",
"'	c #E0E0E0",
")	c #D0D0D0",
"                ",
"                ",
"         .+@@   ",
"       #.+@$%%& ",
"   .+@@*=@$-;;- ",
" #.+@$%%>@%,'',%",
" *=@$-;;->%,'',%",
"#*=@%,'';$&-,,-$",
"#*=@%,));%@&%%&@",
"##=>$-,,-&+>@@> ",
"##*+@$%$$+..=== ",
" ##.+>@@>*##*   ",
" ###*===.       ",
"   ###*         ",
"                ",
"                "};

char *server_xpm_data[] = {
"16 16 17 1",
" 	c None",
".	c #3030C0",
"+	c #4040C0",
"@	c #2020C0",
"#	c #5050C0",
"$	c #6060C0",
"%	c #7070C0",
"&	c #0000C0",
"*	c #8080C0",
"=	c #9090C0",
"-	c #B0B0C0",
";	c #1010C0",
">	c #A0A0C0",
",	c #D0D0D0",
"'	c #E0E0E0",
")	c #C0C0C0",
"!	c #F0F0F0",
"      .+        ",
"    @.#$%$      ",
"  &@.+#%*=-*    ",
"&&;@+#%%=>-,'-  ",
"&&@.+$%=>-),'!  ",
"&&&;+$*=>->-),  ",
"&&&&&@%=%%=>-)  ",
"&&&&&&&+#$%*>-  ",
"&&&&&&&@+#$%=>  ",
"&&&&&&&;@+#$%*  ",
"&&&&&&&&;@+#$$  ",
"  &&&&&&;;@.    ",
"    &&&&&&      ",
"      &&        ",
"                ",
"                "};

/* vi: set ts=3: */

