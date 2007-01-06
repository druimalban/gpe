## Process this file with automake to produce Makefile.in

LFLAGS = -L
YFLAGS = -ld


INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-wlancfg

gpe_wlancfg_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	confpars_wl.y \
	confscan_wl.l \
#	cfgfile.c cfgfile.h \
	config-parser.c config-parser.h
#	confpars_wl.c confpars_wl.h
#	confscan_wl.c confscan_wl.h


# gpe_wlancfg_LDADD = @GTK_LIBS@ $(INTLLIBS) /skiff/local/arm-linux/lib/libfl.a
gpe_wlancfg_LDADD = @GTK_LIBS@ $(INTLLIBS) /usr/lib/libfl.a
