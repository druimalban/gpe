dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-calendar,0.66,gpe@handhelds.org)

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

 dbus-glib-1
AC_MSG_CHECKING([if D-Bus support has been enabled]);
AC_ARG_ENABLE(
  dbus,
  [  --disable-dbus          compile without D-Bus support],
  [ 
    if test "$enable_dbus" = "yes"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  ],
  [ 
    enable_dbus="yes";
    AC_MSG_RESULT(yes) 
  ]
)
if test "$enable_dbus" = "yes"; then
    DEP_MODULES="$DEP_MODULES dbus-glib-1"
    AC_DEFINE([USE_DBUS], [1], [Define to 1 to enable D-Bus support])
fi

ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
	ENABLE_HILDON=true
	DEP_MODULES="$DEP_MODULES hildon-lgpl hildon-libs hildon-fm libosso"
],
[
	ENABLE_HILDON=false
])

AC_SUBST(ENABLE_HILDON)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_CHECK_LIB(dbus-1, dbus_message_iter_get_basic, AC_DEFINE(HAVE_DBUS_MESSAGE_ITER_GET_BASIC, 1, [Define to 1 if you have the dbus_message_iter_get_basic() function.]))

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


AC_OUTPUT([
Makefile
po/Makefile.in
gpe-calendar.desktop.in
hildon/com.nokia.gpe-calendar.service
hildon/gpe-calendar.desktop.in
])
