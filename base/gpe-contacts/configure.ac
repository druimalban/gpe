dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-contacts,0.47,gpe-list@linuxtogo.org)

AC_CONFIG_SRCDIR([main.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.2 libgpewidget libgpepimc libgpevtype libcontactsdb sqlite mimedir-1.0"

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
        PKG_CHECK_MODULES(HILDON, hildon-1,
                [
			AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
			DEP_MODULES="$DEP_MODULES hildon-fm-2"
		],
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-lgpl hildon-libs)
			DEP_MODULES="$DEP_MODULES hildon-fm"
                ])
	DEP_MODULES="$DEP_MODULES libosso"
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
edit-vcard.desktop.in
gpe-contacts.desktop.in
hildon/com.nokia.gpe-contacts.service
hildon/gpe-contacts.desktop.in
])
