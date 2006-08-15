dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-fbpanel,4.4,gpe@handhelds.org)

AC_CONFIG_SRCDIR([panel.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL

DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.2 x11 libgpewidget libgpelaunch"
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""

gpefbpanel_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpefbpanel_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


AC_OUTPUT([
Makefile
po/Makefile.in
plugins/Makefile
systray/Makefile
])
