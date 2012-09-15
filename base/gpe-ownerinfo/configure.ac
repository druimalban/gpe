dnl Process this file with autoconf to produce a configure script.
dnl gpe-ownerinfo-0.28

AC_PREREQ(2.59)
AC_INIT(gpe-ownerinfo,0.28,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"

gpeownerinfo_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpeownerinfo_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

# Checks for programs.
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

# library dependencies
DEP_MODULES="gtk+-2.0 >= 2.4 glib-2.0 >= 2.2 gdk-2.0 gdk-pixbuf-2.0 libgpewidget x11"

# library dependencies
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_SUBST(PREFIX)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
])
