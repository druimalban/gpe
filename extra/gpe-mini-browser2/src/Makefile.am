bin_PROGRAMS = gpe-mini-browser2

gpe_mini_browser2_SOURCES = main.c ui.c ui-callbacks.c utility-functions.c 

CFLAGS = -Os -Wall -g -pedantic -std=c99
LDFLAGS +=  -lstdc++

INCLUDES = @MINIBROWSER_CFLAGS@ -DPREFIX=\"@prefix@\" -Wall
LDADD = @MINIBROWSER_LIBS@


