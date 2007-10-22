dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-edit,0.40,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="cs de es fr hu id ko nl pt_BR pt ro ru rw sk sl sr sv tr vi wa zh_CN zh_TW"

gpeedit_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpeedit_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
gpe-edit.desktop.in
])
