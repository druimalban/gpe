dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-bluetooth,0.43,gpe@handhelds.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget libgpevtype dbus-glib-1 libglade-2.0 mimedir-1.0 gthread-2.0 bluez"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""

gpebluetooth_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpebluetooth_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(GPE_BLUETOOTH, $DEP_MODULES)

AC_SUBST(GPE_BLUETOOTH_CFLAGS)
AC_SUBST(GPE_BLUETOOTH_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
])
