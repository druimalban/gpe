## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-alarm

gpe_alarm_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

gpe_alarm_LDADD = @GTK_LIBS@ $(INTLLIBS) -lgpe -lgthread -lpthread

