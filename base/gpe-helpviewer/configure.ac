#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-helpviewer, 0.5, gpe@handhelds.org)
AC_CONFIG_SRCDIR([gpe-helpviewer.c])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

PKG_CHECK_MODULES(HELPVIEWER, gtk+-2.0 libgpewidget webkit-1.0)

DEFINES="-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
DEFINES="$DEFINES -DNDEBUG"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
