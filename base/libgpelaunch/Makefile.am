lib_LTLIBRARIES = libgpelaunch.la

INCLUDES = @LAUNCHER_CFLAGS@ -DPACKAGE=\"@PACKAGE_NAME@\" -DPACKAGE_LOCALE_DIR=\"@datadir@/locale/@PACKAGE_NAME@\" -DSN_API_NOT_YET_FROZEN

LDADD = @LAUNCHER_LIBS@

libgpelaunch_la_SOURCES = launch.c launch.h desktop_file.c desktop_file.h
