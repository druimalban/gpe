dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-login,0.93,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([gpe-login.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="glib-2.0 >= 2.2 libgpewidget"
DEP_MODULES_LOCK="x11"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="cs ja fr ko sr vi zh_TW de nl ro sk sv wa es hu pt_BR ru sl tr zh_CN rw ky fi"

gpelogin_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpelogin_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
PKG_CHECK_MODULES(DEPS_LOCK, $DEP_MODULES_LOCK)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)
AC_SUBST(DEPS_LOCK_CFLAGS)
AC_SUBST(DEPS_LOCK_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
])
