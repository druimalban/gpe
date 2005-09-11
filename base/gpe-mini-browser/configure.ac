#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-mini-browser, 0.17 philippedeswert@scarlet.be)
AC_CONFIG_SRCDIR([gpe-mini-browser.c])
AM_INIT_AUTOMAKE([foreign])
 
AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

# Hildon stuff

ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
        ENABLE_HILDON=true
        HILDON_DEPS="hildon-lgpl hildon-fm libosso"
],
[
        ENABLE_HILDON=false
])

AC_SUBST(ENABLE_HILDON)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

# bookmark stuff

ENABLE_BOOKMARK=true

AC_ARG_ENABLE(bookmarks,
[  --disable-bookmarks	Disable bookmarks and forget about sqlite dependency],
[
	ENABLE_BOOKMARK=false
	BOOKMARK_DEPS=""
],
[
	ENABLE_BOOKMARK=true
	BOOKMARK_DEPS="sqlite"
])

AC_SUBST(ENABLE_BOOKMARK)

AM_CONDITIONAL(BOOKMARK, test x$ENABLE_BOOKMARK = xtrue)

PKG_CHECK_MODULES(MINIBROWSER, gtk+-2.0 libgpewidget osb-nrcit $HILDON_DEPS $BOOKMARK_DEPS)

DEFINES="-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
DEFINES="$DEFINES -DNDEBUG"

AC_SUBST(MINIBROWSER_CFLAGS)
AC_SUBST(MINIBROWSER_LIBS)

AC_OUTPUT([Makefile])
