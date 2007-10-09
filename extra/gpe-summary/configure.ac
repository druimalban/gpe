AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(gpesummary, 0.6)

AC_CONFIG_HEADER(config.h)

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES(OSSO, libosso >= 0.8.4)
AC_SUBST(OSSO_LIBS)
AC_SUBST(OSSO_CFLAGS)

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.6.4)
AC_SUBST(GCONF_LIBS)
AC_SUBST(GCONF_CFLAGS)

DEP_MODULES="gtk+-2.0 >= 2.6 glib-2.0 >= 2.2 libgpewidget libgpepimc libgpevtype libcontactsdb sqlite mimedir-1.0 libeventdb >= 0.29 libxsettings-client libhandoff libsoup-2.2"

PKG_CHECK_MODULES(EVENTDB, libeventdb >= 0.0.1)
AC_SUBST(EVENTDB_LIBS)
AC_SUBST(EVENTDB_CFLAGS)

PKG_CHECK_MODULES(TODODB, libtododb >= 0.0.1)
AC_SUBST(TODODB_LIBS)
AC_SUBST(TODODB_CFLAGS)

PKG_CHECK_MODULES(LIBCONTACTSDB, libcontactsdb >= 0.0.1)
AC_SUBST(LIBCONTACTSDB_LIBS)
AC_SUBST(LIBCONTACTSDB_CFLAGS)

dnl Set PACKAGE_LOCALE_DIR in config.h
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR], ["${ac_default_prefix}/${DATADIRNAME}/locale"],[Locale directory])
else
  AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR], ["${prefix}/${DATADIRNAME}/locale"], [Locale directory])
fi

AC_PROG_INTLTOOL([0.23])
ALL_LINGUAS="en_GB de_DE en de fi_FI fi"
GETTEXT_PACKAGE=gpesummary
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT()


AC_OUTPUT([ 
Makefile
po/Makefile.in 
])