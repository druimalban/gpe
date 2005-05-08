#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libtododb, 0.09, gpe@handhelds.org)
AC_CONFIG_SRCDIR([todo-db.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(TODODB, sqlite libgpepimc)

#gtk-doc for documentation
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile libtododb.pc doc/Makefile])
AC_OUTPUT
