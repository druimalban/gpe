#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libhandoff, 0.1, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([handoff.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(DEPS, glib-2.0)

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

#gtk-doc for documentation, currently unused
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile libhandoff.pc])
AC_OUTPUT
