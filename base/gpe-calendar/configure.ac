dnl Process this file with autoconf to produce a configure script.

AC_INIT(gpe-calendar,0.92,gpe-list@linuxtogo.org)

AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL
AC_PROG_LIBTOOL

DEP_MODULES="gtk+-2.0 >= 2.4 glib-2.0 >= 2.2 libgpewidget >= 0.109 libgpepimc \
             libgpevtype >= 0.50 libeventdb >= 0.90 \
	     libxsettings-client \
	     libhandoff"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="id pt de fr zh_TW ro wa sv nl cs es sk sr ru hu zh_CN"

gpecalendar_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpecalendar_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

# dbus-glib-1
AC_MSG_CHECKING([if D-Bus support has been enabled]);
AC_ARG_ENABLE(
  dbus,
  [  --disable-dbus          compile without D-Bus support],
  [ 
    if test "$enable_dbus" = "yes"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  ],
  [ 
    enable_dbus="yes";
    AC_MSG_RESULT(yes) 
  ]
)
if test "$enable_dbus" = "yes"; then
    DEP_MODULES="$DEP_MODULES dbus-glib-1"
    AC_DEFINE([USE_DBUS], [1], [Define to 1 to enable D-Bus support])
fi

ENABLE_HILDON=false
AC_ARG_ENABLE(hildon,
	[  --enable-hildon         Enable Hildon GUI],
	[
		ENABLE_HILDON=true
		PKG_CHECK_MODULES(HILDON, hildon-1,
			[
				AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries])
				DEP_MODULES="$DEP_MODULES hildon-fm-2"
			],
			[
				AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
				PKG_CHECK_MODULES(HILDON, hildon-lgpl hildon-libs)
				DEP_MODULES="$DEP_MODULES hildon-fm"
			])
		PKG_CHECK_EXISTS(maemo-version,
			[
				VERSION=`pkg-config --modversion maemo-version`
				AC_MSG_NOTICE([Configuring for Maemo $VERSION])
				CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"$VERSION\\\""
				MAJOR=`echo $VERSION | cut -b1 -`
				CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=$MAJOR"
			],
			[
				AC_MSG_ERROR([maemo-version not found])
			])
		DEP_MODULES="$DEP_MODULES libosso"
	],
	[
		ENABLE_HILDON=false
		CFLAGS="$CFLAGS -DMAEMO_VERSION=\\\"\\\""
		CFLAGS="$CFLAGS -DMAEMO_VERSION_MAJOR=0"
	])
AC_SUBST(ENABLE_HILDON)
AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

# See if we should use libschedule.
AC_ARG_WITH(libschedule,
	[AC_HELP_STRING([--without-libschedule],[disable use of libschedule])])
if test x$with_libschedule != xno
then
  DEP_MODULES="$DEP_MODULES libschedule"
  AC_DEFINE(WITH_LIBSCHEDULE)
fi

# See if we should use libsoundgen.
AC_ARG_WITH(libsoundgen,
	[AC_HELP_STRING([--without-libsoundgen],[disable use of libsoundgen])])
if test x$with_libsoundgen != xno
then
  DEP_MODULES="$DEP_MODULES libsoundgen >= 0.5"
  AC_DEFINE(WITH_LIBSOUNDGEN)
fi

# Check which version of libsoup is installed
AC_MSG_CHECKING([which libsoup version to use]);
PKG_CHECK_EXISTS([libsoup-2.4],
			   [
			   AC_MSG_RESULT(2.4)
			   AC_DEFINE(LIBSOUP_VER, 24, [Version of libsoup library])
			   AC_DEFINE(LIBSOUP24, 1, [Using libsoup-2.4 library])
			   DEP_MODULES="$DEP_MODULES libsoup-2.4"
			   ],
			   [
			   AC_MSG_RESULT(2.2)
			   AC_DEFINE(LIBSOUP_VER, 22, [Version of libsoup library])
			   AC_DEFINE(LIBSOUP22, 1, [Using libsoup-2.2 library])
			   DEP_MODULES="$DEP_MODULES libsoup-2.2"
			   ])

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)

AC_REPLACE_FUNCS([g_date_set_time_t])

AC_CHECK_LIB(dbus-1, dbus_message_iter_get_basic, AC_DEFINE(HAVE_DBUS_MESSAGE_ITER_GET_BASIC, 1, [Define to 1 if you have the dbus_message_iter_get_basic() function.]))

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)


AC_OUTPUT([
Makefile
po/Makefile.in
po/Makefile
pixmaps/Makefile
gpe-calendar.desktop.in
hildon/com.nokia.gpe-calendar.service
hildon/gpe-calendar.desktop.in
])
