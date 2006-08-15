lib_LTLIBRARIES = libplugins.la

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) -DSTATIC_PLUGINS\
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpefbpanel_localedir@\"

LDADD = @DEPS_LIBS@

libplugins_la_LIBADD = @DEPS_LIBS@
libplugins_la_SOURCES =  menu.c separator.c taskbar.c wincmd.c \
                      dclock.c launchbar.c pager.c space.c \
                      icons.c deskno.c cpu.c
