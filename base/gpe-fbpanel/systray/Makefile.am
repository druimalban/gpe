lib_LTLIBRARIES = libsystray.la

INCLUDES = $(DEPS_CFLAGS) -I$(top_srcdir) -DSTATIC_PLUGINS\
           -DPREFIX=\"@prefix@\" -D_GNU_SOURCE -Wall \
           -DPACKAGE_LOCALE_DIR=\"@gpefbpanel_localedir@\"
           
LDADD = @DEPS_LIBS@

libsystray_la_LIBADD = @DEPS_LIBS@
libsystray_la_SOURCES = eggmarshalers.c eggmarshalers.h\
                        eggtraymanager.c \
                        fixedtip.c systray.c
