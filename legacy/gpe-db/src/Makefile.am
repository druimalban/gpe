## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-db

gpe_db_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	db-funcs.c db-funcs.h

gpe_db_LDADD = -L../../libgpewidget -lgpewidget @GTK_LIBS@ $(INTLLIBS) -lsqlite

