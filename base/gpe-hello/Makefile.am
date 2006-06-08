bin_PROGRAMS = gpe-hello

INCLUDES = @GPEHELLO_CFLAGS@ -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall 
LDADD = @GPEHELLO_LIBS@

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = gpe-hello.png

EXTRA_DIST = gpe-dist.am \
             $(desktop_DATA) \
             $(pixmaps_DATA)
                                                                                                       intltool-update.in \
include gpe-dist.am
