#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpelauncher, VERSION, BUG-REPORT-ADDRESS)
AC_CONFIG_SRCDIR([launch.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_MALLOC

PKG_CHECK_MODULES(LAUNCHER, gtk+-2.0 libstartup-notification-1.0)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
