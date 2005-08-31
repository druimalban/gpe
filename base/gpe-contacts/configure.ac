dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-contacts,0.42,gpe@handhelds.org)

AC_CONFIG_SRCDIR([main.c])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.2 libgpewidget libgpepimc libgpevtype sqlite mimedir-1.0"

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
edit-vcard.desktop.in
gpe-contacts.desktop.in
hildon/org.gpe.gpe-contacts.service
hildon/gpe-contacts.desktop.in
])
