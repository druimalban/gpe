## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-ownerinfo

gpe_ownerinfo_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

gpe_ownerinfo_LDADD = @GTK_LIBS@ $(INTLLIBS)

