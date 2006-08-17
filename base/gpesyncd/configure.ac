AC_INIT(gpesyncd,0.1,martin@silef.de)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_HEADER_STDC
AC_PROG_CXX

DEP_MODULES="glib-2.0 mimedir-1.0 libgpevtype sqlite gobject-2.0 gthread-2.0 gmodule-2.0"

AC_PROG_LIBTOOL

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
AC_SUBST(DEPS_FLAGS)
AC_SUBST(DEPS_FLAGS)

AC_OUTPUT([
Makefile
src/Makefile
])
