#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libschedule, 0.14, gpe@handhelds.org)
AC_CONFIG_SRCDIR([atd.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(SCHEDULE, glib-2.0)

#gtk-doc for documentation, currently unused
GTK_DOC_CHECK(1.2)

AC_CONFIG_FILES([Makefile doc/Makefile libschedule.pc])
AC_OUTPUT
