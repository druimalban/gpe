libgpelaunchdir = $(libexecdir)/libgpelaunch

lib_LTLIBRARIES = libgpelaunch.la
libgpelaunch_PROGRAMS = launch-failure

INCLUDES = @LAUNCHER_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -DSN_API_NOT_YET_FROZEN -DLIBEXECDIR=\"$(libexecdir)\"

libgpelaunch_la_LIBADD = @LAUNCHER_LIBS@
libgpelaunch_la_SOURCES = launch.c gpe/launch.h desktop_file.c gpe/desktop_file.h

launch_failure_LDADD = @LAUNCHER_LIBS@

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = gpe/launch.h gpe/desktop_file.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpelaunch.pc

include gpe-dist.am
