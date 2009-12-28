AC_INIT(starling, 0.9.9,[gpe-list@linuxtogo.org])
AC_CONFIG_SRCDIR([src/starling.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_PROG_INSTALL
AC_PROG_CC
AC_LANG(C)
AC_PROG_YACC

dnl i18n stuff
ALL_LINGUAS=""
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/share/locale", 
				       [Locale-specific data directory])
AC_SUBST(starling_localedir)
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

BASE_MODULES="glib-2.0 gstreamer-0.10 gstreamer-plugins-base-0.10"

GUI_MODULES="gtk+-2.0 libhandoff"

dnl Optional stuff
AC_ARG_ENABLE(gpe, [ --enable-gpe   Enable GPE support], [gpe=$enableval], [gpe=no])
dnl Check for GPE
if test "x"$gpe = "xyes"; then
  GUI_MODULES="$GUI_MODULES libgpewidget >= 0.102"
  AC_DEFINE([ENABLE_GPE], [1], [Define to 1 to enable GPE widgets])
fi

PKG_PROG_PKG_CONFIG

# Whether to build for a Hildon-based desktop.
AC_MSG_CHECKING([for a Hildon Desktop environment])
PKG_CHECK_EXISTS(libosso,
  [
    GUI_MODULES="$GUI_MODULES libosso"
  
    HAVE_HILDON_VERSION=0
    PKG_CHECK_EXISTS(hildon-1,
      [
        HAVE_HILDON_VERSION_RAW=$(pkg-config --modversion hildon-1)
        # Extract the version as four digit number, AABB where AA is the
        # major version number and BB is the minor version number.
        HAVE_HILDON_VERSION=$(echo "$HAVE_HILDON_VERSION_RAW" \
          | awk 'BEGIN { FS="." }
                 /[0-9]+([.][0-9]+)?([.][0-9]+)?/
                   { printf ("%d%02d", $1, $2); }
                ')
        GUI_MODULES="$GUI_MODULES hildon-1 hildon-fm-2"
      ],
      [
        HAVE_HILDON_VERSION=0
        GUI_MODULES="$GUI_MODULES hildon-fm hildon-lgpl hildon-libs"
      ])
      AC_MSG_RESULT([yes, version $HAVE_HILDON_VERSION])
      AC_DEFINE_UNQUOTED([HAVE_HILDON_VERSION], $HAVE_HILDON_VERSION,
        [Version of Hildon (format: XXYY, XX: Major, Y: Minor).])
  ],
  [
    AC_MSG_RESULT([no])
  ])
HAVE_HILDON=$(if test x$HAVE_HILDON_VERSION = x; then echo 0; else echo 1; fi)
AC_DEFINE_UNQUOTED(HAVE_HILDON,
	[$HAVE_HILDON],
	[Whether we are building for Hildon.])
AM_CONDITIONAL([HILDON], test x$HAVE_HILDON = x1)

# Check whether we are running on Maemo and which version.
AC_MSG_CHECKING([for a Maemo environment])
PKG_CHECK_EXISTS(maemo-version,
	[
		HAVE_MAEMO_VERSION_RAW=$(pkg-config --modversion maemo-version)
		AC_MSG_RESULT([yes, version $HAVE_MAEMO_VERSION_RAW])
	        # Extract the version as four digit number, AABB where
	        # AA is the major version number and BB is the minor
	        # version number.
		HAVE_MAEMO_VERSION=$(echo "$HAVE_MAEMO_VERSION_RAW" \
		   | awk 'BEGIN { FS="." }
		          /[0-9]+([.][0-9]+)?([.][0-9]+)?/
                            { printf ("%d%02d", $1, $2); }
                         ')
                AC_DEFINE_UNQUOTED([HAVE_MAEMO_VERSION], $HAVE_MAEMO_VERSION,
                          [Version of Maemo (format: XXYY, XX: Major, Y: Minor).])
	],
	[
		AC_MSG_RESULT([no, configuring for a GTK-based environment])
	])
HAVE_MAEMO=$(if test x$HAVE_MAEMO_VERSION = x; then echo 0; else echo 1; fi)
AC_DEFINE_UNQUOTED(HAVE_MAEMO,
	[$HAVE_MAEMO],
	[Whether we are building for Maemo.])
AM_CONDITIONAL([MAEMO], [test 0$HAVE_MAEMO_VERSION -gt 0])
AM_CONDITIONAL([MAEMO5], [test 0$HAVE_MAEMO_VERSION -ge 500])


# Check which version of libsoup is installed
AC_MSG_CHECKING([which libsoup version to use])
PKG_CHECK_EXISTS([libsoup-2.4],
			   [
			   AC_MSG_RESULT(2.4)
			   AC_DEFINE(LIBSOUP_VER, 24, [Version of libsoup library])
			   AC_DEFINE(LIBSOUP24, 1, [Using libsoup-2.4 library])
			   GUI_MODULES="$GUI_MODULES libsoup-2.4"
			   ],
			   [
			   AC_MSG_RESULT(2.2)
			   AC_DEFINE(LIBSOUP_VER, 22, [Version of libsoup library])
			   AC_DEFINE(LIBSOUP22, 1, [Using libsoup-2.2 library])
			   GUI_MODULES="$GUI_MODULES libsoup-2.2"
			   ])


# Check which version of libsqlite is installed (prefer version 3).
AC_MSG_CHECKING([which sqlite to use]);
PKG_CHECK_EXISTS([sqlite3],
		 [
		   AC_MSG_RESULT(sqlite3)
		   AC_DEFINE(HAVE_SQLITE_VERSION, 3, [Version of sqlite])
		   BASE_MODULES="$BASE_MODULES sqlite3"
		 ],
		 [
		   AC_MSG_RESULT(sqlite2)
		   AC_DEFINE(HAVE_SQLITE_VERSION, 2, [Version of sqlite])
		   BASE_MODULES="$BASE_MODULES sqlite >= 2.8"
		 ])

PKG_CHECK_MODULES(BASE, $BASE_MODULES)
AC_SUBST(BASE_CFLAGS)
AC_SUBST(BASE_LIBS)

GUI_MODULES="$GUI_MODULES $BASE_MODULES"
PKG_CHECK_MODULES(GUI, $GUI_MODULES)
AC_SUBST(GUI_CFLAGS)
AC_SUBST(GUI_LIBS)

old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $GUI_CFLAGS"
old_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $GUI_CFLAGS"
old_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $GUI_LIBS"
AC_CHECK_FUNCS([gtk_volume_button_new])
AC_CHECK_FUNCS([g_uri_escape_string])
AC_CHECK_FUNCS([g_compute_checksum_for_data], [],
               [AC_CHECK_FUNCS([md5_buffer], [], [AC_LIBOBJ([md5])])])
AC_REPLACE_FUNCS([gst_bus_timed_pop_filtered])
AC_REPLACE_FUNCS([gst_bus_timed_pop])
AC_CHECK_FUNC([hildon_app_menu_append], [AC_DEFINE(HAVE_HILDON_APP_MENU, 1)])
CFLAGS="$old_CFLAGS"
CPPFLAGS="$old_CPPFLAGS"
LDFLAGS="$old_LDFLAGS"

dnl glib-genmarshal
GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

dnl Generate the output files.
AC_CONFIG_FILES([
po/Makefile.in
Makefile
data/Makefile
data/starling.desktop.in
src/Makefile
hildon/starling.desktop
hildon/com.nokia.starling.service
hildon/Makefile
])
AC_OUTPUT
