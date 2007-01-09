## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-config-sleep

gpe_config_sleep_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	confGUI.c confGUI.h \
	conf.c conf.h 

gpe_config_sleep_LDADD = @GTK_LIBS@

