#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libeventdb, 0.90, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([event-db.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(EVENTDB, gobject-2.0 glib-2.0 sqlite)
AC_SUBST(EVENTDB_CFLAGS)
AC_SUBST(EVENTDB_LIBS)

#gtk-doc for documentation
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile doc/Makefile libeventdb.pc])
AC_OUTPUT
