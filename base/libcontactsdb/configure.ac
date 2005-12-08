#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libcontactsdb, 0.1, linuxipaq@psychogenic.com)
AC_CONFIG_SRCDIR([contacts-db.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(CONTACTSDB, sqlite libgpepimc)

AC_CONFIG_FILES([Makefile libcontactsdb.pc])
AC_OUTPUT
