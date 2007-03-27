dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-aerial,0.2.13,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget >= 0.114 gthread-2.0 sqlite"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="de sv ro cs nl pt ru fr sr es pt_BR vi rw sl"

gpeaerial_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpeaerial_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
gpe-aerial.desktop.in
])
