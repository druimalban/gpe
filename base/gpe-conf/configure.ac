dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-conf,0.2.9,gpe-list@linuxtogo.org)

AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

DEP_MODULES="gtk+-2.0 >= 2.4 glib-2.0 >= 2.2 libgpewidget >= 0.109 x11 xrandr libxsettings-client audiofile esound"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"

gpeconf_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpeconf_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


AC_OUTPUT([
Makefile
po/Makefile.in
])
