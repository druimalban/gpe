lib_LTLIBRARIES = libgpelaunch.la

INCLUDES = @LAUNCHER_CFLAGS@ -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -DSN_API_NOT_YET_FROZEN

libgpelaunch_la_LIBADD = @LAUNCHER_LIBS@

libgpelaunch_la_SOURCES = launch.c launch.h desktop_file.c desktop_file.h

gpeincludedir = $(includedir)/gpe
gpeinclude_HEADERS = launch.h desktop_file.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpelaunch.pc

include gpe-dist.am
