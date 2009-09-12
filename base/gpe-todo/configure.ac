dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-todo,0.58,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget >= 0.114 libtododb libgpepimc >= 0.8"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"

gpetodo_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpetodo_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
	ENABLE_HILDON=true
        PKG_CHECK_MODULES(HILDON, hildon-1,
                AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-lgpl)
                ])
	PKG_CHECK_EXISTS(maemo-version,
		[
			VERSION=`pkg-config --modversion maemo-version`
			AC_MSG_NOTICE([Configuring for Maemo $VERSION])
			CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"$VERSION\\\""
			MAEMO_MAJOR=`echo $VERSION | cut -b1 -`
			CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=$MAEMO_MAJOR"
		],
		[
			AC_MSG_ERROR([maemo-version not found])
		])
	DEP_MODULES="$DEP_MODULES libosso"
],
[
	ENABLE_HILDON=false
	CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"\\\""
	CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=0"
])

AC_SUBST(ENABLE_HILDON)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
AM_CONDITIONAL(MAEMO5, test x$MAEMO_MAJOR = x5)

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
hildon/Makefile
gpe-todo.desktop.in
hildon/com.nokia.gpe-todo.service
hildon/gpe-todo.desktop.in
])
