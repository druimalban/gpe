#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgtkstylus, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([tap.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(STYLUS, gtk+-2.0)

GTK_VERSION=`$PKG_CONFIG --variable=gtk_binary_version gtk+-2.0`
GTK_MODULES_DIR="${libdir}/gtk-2.0/$GTK_VERSION/modules"

AC_SUBST(GTK_MODULES_DIR)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
