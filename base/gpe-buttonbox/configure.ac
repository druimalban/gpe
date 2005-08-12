#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-buttonbox, 0.4, gpe@handhelds.org)
AC_CONFIG_SRCDIR([gpe-buttonbox.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

PKG_CHECK_MODULES(BUTTONBOX, gtk+-2.0 libgpewidget libgpelaunch)

AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "$prefix/share/locale/$PACKAGE_NAME", [Directory for locale files])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
