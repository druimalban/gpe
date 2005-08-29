dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-calendar,0.64,gpe@handhelds.org)

AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.4 glib-2.0 >= 2.2 libgpewidget libgpepimc \
             libgpevtype sqlite mimedir-1.0 libschedule libeventdb libxsettings-client"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="id pt de fr zh_TW ro wa sv nl cs es sk sr ru hu zh_CN"

gpecalendar_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpecalendar_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
	ENABLE_HILDON=true
	DEP_MODULES="$DEP_MODULES hildon-lgpl hildon-fm libosso"
],
[
	ENABLE_HILDON=false
])

AC_SUBST(ENABLE_HILDON)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


AC_OUTPUT([
Makefile
po/Makefile.in
gpe-calendar.desktop.in
hildon/org.gpe.gpe-calendar.service
hildon/gpe-calendar.desktop.in
])
