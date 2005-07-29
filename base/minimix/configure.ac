#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(minimix, 0.7, gpe@handhelds.org)
AC_CONFIG_SRCDIR([minimix.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES(MINIMIX, gtk+-2.0 libgpewidget gdk-pixbuf-xlib-2.0)

AC_DEFINE_UNQUOTED(PREFIX, "@prefix@", [Installation prefix])
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "$prefix/share/locale/$PACKAGE_NAME", [Directory for locale files])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
