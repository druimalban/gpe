AC_INIT(gpesyncd,2.0,g+gpe@cobb.uk.net)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_GNU_SOURCE
AC_HEADER_STDC
AC_PROG_CXX

DEP_MODULES="glib-2.0 mimedir-1.0 libgpevtype >= 0.20 sqlite gobject-2.0 gthread-2.0 gmodule-2.0 libcontactsdb libeventdb >= 0.31 libtododb >= 0.11"

AC_PROG_LIBTOOL

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
AC_SUBST(DEPS_FLAGS)
AC_SUBST(DEPS_FLAGS)

AC_ARG_ENABLE(vartmp,
[[  --enable-vartmp         Use /var/tmp instead of /tmp for temporary files [default=yes]]],
[
        use_vartmp=$enableval
],
[
        use_vartmp=yes
])
if test "x$use_vartmp" = xyes ; then
AC_DEFINE(USE_VARTMP,1,[Use /var/tmp for temporary files])
fi

AC_OUTPUT([
Makefile
src/Makefile
])
