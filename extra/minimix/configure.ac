#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(minimix, 0.9, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([minimix.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])


# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

PKG_CHECK_MODULES(MINIMIX, gtk+-2.0 libgpewidget gdk-pixbuf-xlib-2.0)

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""

minimix_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(minimix_localedir)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
