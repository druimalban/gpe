#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libschedule, 0.17, gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([atd.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(SCHEDULE, glib-2.0 sqlite3 libgpewidget)

ENABLE_HAVEAT=false

AC_ARG_ENABLE(haveat,
[  --enable-haveat         We have a traditional at command (default no)],
[
	ENABLE_HAVEAT=true
],
[
	ENABLE_HAVEAT=false
])

AC_SUBST(ENABLE_HAVEAT)

AM_CONDITIONAL(HAVEAT, test x$ENABLE_HAVEAT = xtrue)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

#gtk-doc for documentation, currently unused
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile doc/Makefile libschedule.pc])
AC_OUTPUT
