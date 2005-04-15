#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpepimc, 0.4, gpe@handhelds.org)
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

#gtk-doc for documentation
GTK_DOC_CHECK(1.0)

AC_CONFIG_FILES([Makefile libgpepimc.pc po/Makefile.in doc/Makefile])
AC_OUTPUT
