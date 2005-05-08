dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-contacts,0.38,gpe@handhelds.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.2 libgpewidget libgpepimc libgpevtype sqlite dbus-glib-1 mimedir-1.0"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="cs es hu pt_BR ro rw sr vi zh_TW de fr nl pt ru sk sv zh_CN"

gpecontacts_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpecontacts_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
gpe-contacts.desktop.in
edit-vcard.desktop.in
])
