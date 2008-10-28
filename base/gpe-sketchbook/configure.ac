dnl Process this file with autoconf to produce a configure script.
dnl gpe-sketchbook-0.2.9

AC_PREREQ(2.59)
AC_INIT(gpe-sketchbook,0.2.9,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([src/gpe-sketchbook.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"

gpesketchbook_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpesketchbook_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

# Checks for programs.
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

# library dependencies
DEP_MODULES="gtk+-2.0 >= 2.4 libgpewidget >= 0.82 libxsettings-client sqlite"

# library dependencies
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_SUBST(PREFIX)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
gpe-sketchbook.desktop.in
])
