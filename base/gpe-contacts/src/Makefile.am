## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/../libgpewidget \
	@GTK_CFLAGS@

bin_PROGRAMS = adb

adb_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	structure.c

adb_LDADD = @GTK_LIBS@ -L$(top_srcdir)/../libgpewidget -lgpewidget
