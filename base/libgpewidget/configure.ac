#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpewidget, 0.118, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([init.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=libgpewidget
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"

libgpewidgetlocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(libgpewidgetlocaledir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

#gtk-doc for documentation
GTK_DOC_CHECK(1.2)

# library dependencies
DEPS="gtk+-2.0 >= 2.4 glib-2.0 >= 2.6"

AC_ARG_ENABLE(cairo,
[  --enable-cairo         Enable using Cairo rendering toolkit],
[
	ENABLE_CAIRO=$enableval
],
[
       	PKG_CHECK_EXISTS(cairo, [ENABLE_CAIRO=yes], [ENABLE_CAIRO=no])
])

ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
	ENABLE_HILDON=true
        PKG_CHECK_MODULES(HILDON, hildon-1,
                AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
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
],
[
	ENABLE_HILDON=false
	CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"\\\""
	CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=0"
])

AC_SUBST(ENABLE_HILDON)
AC_SUBST(ENABLE_CAIRO)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
AM_CONDITIONAL(CAIRO, test x$ENABLE_CAIRO = xyes)
AM_CONDITIONAL(MAEMO5, test x$MAEMO_MAJOR = x5)

if test x$ENABLE_CAIRO = xyes; then
    DEPS="$DEPS cairo >= 0.4.0"
fi

PKG_CHECK_MODULES(GPEWIDGET, $DEPS)

AC_SUBST(PREFIX)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([Makefile libgpewidget.pc 
                 libgpewidget-uninstalled.pc 
				 doc/Makefile po/Makefile.in])
AC_OUTPUT

echo "
######################################
Configuration:

- Compiler...........: ${CC}
- Cairo..............: ${ENABLE_CAIRO}
- Documentation......: ${enable_gtk_doc}

######################################

Now type 'make' to compile ${GETTEXT_PACKAGE}
"
