#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-helpviewer, 0.1, gpe@handhelds.org)
AC_CONFIG_SRCDIR([gpe-helpviewer.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

PKG_CHECK_MODULES(HELPVIEWER, gtk+-2.0 libgpewidget libgpelaunch libgtkhtml-3.0)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
