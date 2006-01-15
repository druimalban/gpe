#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(libgpelaunch, 0.14, gpe@handhelds.org)
AC_CONFIG_SRCDIR([launch.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(LAUNCHER, glib-2.0 libstartup-notification-1.0)
PKG_CHECK_MODULES(LAUNCHER_GTK, gtk+-2.0)

AC_CONFIG_FILES([Makefile libgpelaunch.pc])
AC_OUTPUT
