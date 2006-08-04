#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libcontactsdb, 0.5, gpe@handhelds.org)
AC_CONFIG_SRCDIR([contacts-db.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

GTK_DOC_CHECK(1.2)

PKG_CHECK_MODULES(CONTACTSDB, sqlite libgpepimc)

AC_CONFIG_FILES([Makefile libcontactsdb.pc doc/Makefile])
AC_OUTPUT
