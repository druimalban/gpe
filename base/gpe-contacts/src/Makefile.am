## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/../libgpewidget \
	@GTK_CFLAGS@ `xml2-config --cflags`

bin_PROGRAMS = adb

adb_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	structure.c db.c

adb_LDADD = -L$(top_srcdir)/../libgpewidget -lgpewidget -lsqlite -lxml2 @GTK_LIBS@
