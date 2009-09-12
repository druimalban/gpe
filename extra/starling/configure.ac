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

BASE_MODULES="glib-2.0 sqlite >= 2.8 \
	gstreamer-0.10 gstreamer-plugins-base-0.10"

GUI_MODULES="${BASE_MODULES} gtk+-2.0 libhandoff"

dnl Optional stuff
AC_ARG_ENABLE(gpe, [ --enable-gpe   Enable GPE support], [gpe=$enableval], [gpe=no])
dnl Check for GPE
if test "x"$gpe = "xyes"; then
  GUI_MODULES="$GUI_MODULES libgpewidget >= 0.102"
  AC_DEFINE([ENABLE_GPE], [1], [Define to 1 to enable GPE widgets])
fi

# Whether to build for Hildon.
AC_CHECK_LIB([osso], [osso_initialize],
	[AC_MSG_NOTICE([building for Hildon])
	 AC_DEFINE([IS_HILDON], [1], [Define to 1 to integrate with Hildon])
	 ENABLE_HILDON=true
	 PKG_CHECK_MODULES(HILDON, hildon-1,
		[
                AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries])
		GUI_MODULES="$GUI_MODULES hildon-fm-2"
		],
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-lgpl hildon-libs)
			GUI_MODULES="$GUI_MODULES hildon-fm"
                ])
	 PKG_CHECK_EXISTS(maemo-version,
		[
			VERSION=`pkg-config --modversion maemo-version`
			AC_MSG_NOTICE([Configuring for Maemo $VERSION])
			CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"$VERSION\\\""
			MAEMO_MAJOR=`echo $VERSION | cut -b1 -`
			CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=$MAEMO_MAJOR"
		],
		[
			AC_MSG_ERROR([maemo-version not found])
		])
	 GUI_MODULES="$GUI_MODULES libosso"],
	[ENABLE_HILDON=false])
AC_SUBST(ENABLE_HILDON)
AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)
AM_CONDITIONAL(MAEMO5, test x$MAEMO_MAJOR = x5)
AC_SUBST(HILDON_CFLAGS)
AC_SUBST(HILDON_LIBS)

# Check which version of libsoup is installed
AC_MSG_CHECKING([which libsoup version to use]);
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

PKG_CHECK_MODULES(BASE, $BASE_MODULES)
AC_SUBST(BASE_CFLAGS)
AC_SUBST(BASE_LIBS)

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
