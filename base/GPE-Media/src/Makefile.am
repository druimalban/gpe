## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@ -D_GNU_SOURCE

bin_PROGRAMS = gpe-media

gpe_media_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	io_madplay.c

gpe_media_LDADD = @GTK_LIBS@ $(INTLLIBS)
