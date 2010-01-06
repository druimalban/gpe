#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpepimc, 0.9, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([ui.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=libgpepimc
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""

libgpepimclocaledir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(libgpepimclocaledir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# depends on...
DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 sqlite libgpewidget"


ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
        ENABLE_HILDON=true
        PKG_CHECK_MODULES(HILDON, hildon-1,
                AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-libs)
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
])

PKG_CHECK_MODULES(PIMC, $DEP_MODULES)

if test x$ENABLE_HILDON = xtrue; then
    PIMC_CFLAGS="$PIMC_CFLAGS -DIS_HILDON"
fi

GTK_DOC_CHECK(1.2)

AC_SUBST(ENABLE_HILDON)
AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
AM_CONDITIONAL(MAEMO5, test x$MAEMO_MAJOR = x5)

AC_CONFIG_FILES([Makefile libgpepimc.pc po/Makefile.in doc/Makefile])
AC_OUTPUT
