## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir)/intl -I/usr/src/linux-ipaq-arm/kernel/include \
	@GTK_CFLAGS@

bin_PROGRAMS = gpe-batmon

gpe_batmon_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h

gpe_batmon_LDADD = @GTK_LIBS@ $(INTLLIBS)

