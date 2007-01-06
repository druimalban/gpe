## Process this file with automake to produce Makefile.in

PREFIX=/usr/local

INCLUDES = \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/../libgpewidget `gdk-pixbuf-config --cflags` \
	@GTK_CFLAGS@ -DPREFIX=\"$(PREFIX)\"

bin_PROGRAMS = gpe-ownerinfo

gpe_ownerinfo_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

gpe_ownerinfo_LDFLAGS = `gdk-pixbuf-config --libs`

gpe_ownerinfo_LDADD = -L$(top_srcdir)/../libgpewidget -lgpewidget \
	@GTK_LIBS@ $(INTLLIBS)
