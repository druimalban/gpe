#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-mini-browser2, 0.01, philippedeswert@gmail.com)
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([foreign])
 
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
#AC_PROG_LIBTOOL

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(MINIBROWSER, gtk+-2.0 libgpewidget webkit-1.0 )

DEFINES="-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
DEFINES="$DEFINES -DNDEBUG"

AC_SUBST(MINIBROWSER_CFLAGS)
AC_SUBST(MINIBROWSER_LIBS)

AC_OUTPUT([Makefile src/Makefile po/Makefile.in gpe-mini-browser2.desktop.in ])
