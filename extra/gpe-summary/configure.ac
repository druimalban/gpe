AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(gpesummary, 0.6)

AC_CONFIG_HEADER(config.h)

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG()

PKG_CHECK_MODULES(HILDON, hildon-1 >= 0.9.9 libhildonmime,
                AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.12.0 libossomime)
                ])
AC_SUBST(HILDON)
AC_SUBST(HILDON_LIBS)
AC_SUBST(HILDON_CFLAGS)

DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 gconf-2.0 >= 2.6.4 libeventdb libtododb libcontactsdb libgpewidget >= 0.114 dbus-glib-1 gnome-vfs-2.0 gnome-vfs-module-2.0 libosso >= 0.8.4"
AC_SUBST(ENABLE_HILDON)

PKG_CHECK_EXISTS(maemo-version,
	[
		VERSION=`pkg-config --modversion maemo-version`
		AC_MSG_NOTICE([Configuring for Maemo $VERSION])
		CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"$VERSION\\\""
		MAJOR=`echo $VERSION | cut -b1 -`
		CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=$MAJOR"
		if test "$MAJOR" -ge 5 ; then
		   DEP_MODULES="$DEP_MODULES libhildondesktop-1"
		   APPLETDIR=hildon-desktop
		   DESKTOPFILE=homegpesummary.hdhomepluginitem
		else
		   APPLETDIR=hildon-home
		   DESKTOPFILE=homegpesummary.hildonhomeapplet
		fi
	],
	[
		AC_MSG_ERROR([maemo-version not found])
	])
AC_SUBST(APPLETDIR)
AC_SUBST(DESKTOPFILE)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


dnl Set PACKAGE_LOCALE_DIR in config.h
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR], ["${ac_default_prefix}/${DATADIRNAME}/locale"],[Locale directory])
else
  AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR], ["${prefix}/${DATADIRNAME}/locale"], [Locale directory])
fi

AC_PROG_INTLTOOL([0.23])
ALL_LINGUAS="en_GB de_DE en de fi_FI fi sv"
GETTEXT_PACKAGE=gpesummary
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT()





AC_OUTPUT([ 
Makefile
po/Makefile.in 
])
