## Process this file with automake to produce Makefile.in

INCLUDES = \
        -I$(top_srcdir)/../libgpewidget `gdk-pixbuf-config --cflags` \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = ipaqscreen

ipaqscreen_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	brightness.c brightness.h \
	calibrate.c calibrate.h \
	rotation.c rotation.h

ipaqscreen_LDADD = -L$(top_srcdir)/../libgpewidget -lgpewidget @GTK_LIBS@ $(INTLLIBS)
