## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = ipaqscreen

ipaqscreen_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

ipaqscreen_LDADD = @GTK_LIBS@ $(INTLLIBS)

