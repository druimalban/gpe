dnl Process this file with autoconf to produce a configure script.
dnl xst-0.15

AC_PREREQ(2.59)
AC_INIT(xst,0.15,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([xst.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="glib-2.0 >= 2.2 libxsettings-client"

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
])
