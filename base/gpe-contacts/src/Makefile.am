## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/../libgpewidget `gdk-pixbuf-config --cflags` \
	@GTK_CFLAGS@ `xml2-config --cflags` -DPREFIX=\"@prefix@\"

bin_PROGRAMS = gpe-contacts

gpe_contacts_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	structure.c structure.h \
	db.c db.h \
	editing.c

gpe_contacts_LDADD = -L$(top_srcdir)/../libgpewidget -lgpewidget -lsqlite -lxml2 @GTK_LIBS@
