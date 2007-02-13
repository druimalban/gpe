#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpevtype, 0.5, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([tag-db.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_GNU_SOURCE
AC_PROG_LIBTOOL

# depends on...
DEP_MODULES="glib-2.0 >= 2.2 mimedir-1.0 libeventdb >= 0.9 libtododb"

PKG_CHECK_MODULES(VTYPE, $DEP_MODULES)

#gtk-doc for documentation
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile libgpevtype.pc libgpevtype-uninstalled.pc doc/Makefile])
AC_OUTPUT
