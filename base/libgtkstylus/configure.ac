#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgtkstylus, 0.1, gpe@handhelds.org)
AC_CONFIG_SRCDIR([tap.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(STYLUS, gtk+-2.0)

GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
GTK_MODULES_DIR="${libdir}/gtk-2.0/$GTK_VERSION/modules"

AC_SUBST(GTK_MODULES_DIR)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
