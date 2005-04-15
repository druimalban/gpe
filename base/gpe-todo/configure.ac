dnl Process this file with autoconf to produce a configure script.

PACKAGE=gpe-todo
VERSION=0.52

AC_PREREQ(2.59)
AC_INIT(gpe-todo,0.52,gpe@handhelds.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget libtododb"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="cs de es fr id nl pt_BR pt ro ru sk sr sv vi wa zh_TW"

gpetodo_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpetodo_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
gpe-todo.desktop.in
])
