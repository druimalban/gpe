AC_INIT(starling, 0.2,[skyhusker@handhelds.org])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_PROG_INSTALL
AC_PROG_CC
AC_LANG(C)

dnl i18n stuff
ALL_LINGUAS=""
AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/share/locale", 
				       [Locale-specific data directory])
AC_SUBST(starling_localedir)
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

DEP_MODULES="gtk+-2.0 gstreamer-0.10 gstreamer-plugins-base-0.10 \
 sqlite >= 2.8 libsoup-2.2"

dnl Optional stuff
AC_ARG_ENABLE(gpe, [ --enable-gpe   Enable GPE support], [gpe=$enableval], [gpe=no])
dnl Check for GPE
if test "x"$gpe = "xyes"; then
  DEP_MODULES="$DEP_MODULES libgpewidget >= 0.102"
  AC_DEFINE([ENABLE_GPE], [1], [Define to 1 to enable GPE widgets])
fi

# Whether to build for Hildon.
AC_CHECK_LIB([osso], [osso_initialize],
	[AC_MSG_NOTICE([building for Hildon])
	 AC_DEFINE([IS_HILDON], [1], [Define to 1 to integrate with Hildon])
	 ENABLE_HILDON=true
	 DEP_MODULES="$DEP_MODULES hildon-lgpl hildon-libs hildon-fm libosso"],
	[ENABLE_HILDON=false])
AC_SUBST(ENABLE_HILDON)
AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

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