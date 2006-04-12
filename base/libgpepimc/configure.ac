#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpepimc, 0.7, gpe@handhelds.org)
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


PKG_CHECK_MODULES(PIMC, $DEP_MODULES)

ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
        ENABLE_HILDON=true
        DEP_MODULES="$DEP_MODULES hildon-lgpl"
],
[
         ENABLE_HILDON=false
])

if test x$ENABLE_HILDON = xtrue; then
    PIMC_CFLAGS="$PIMC_CFLAGS -DIS_HILDON"
fi

GTK_DOC_CHECK(1.2)

AC_SUBST(ENABLE_HILDON)
AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

AC_CONFIG_FILES([Makefile libgpepimc.pc po/Makefile.in doc/Makefile])
AC_OUTPUT
