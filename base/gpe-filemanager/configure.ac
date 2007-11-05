dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-filemanager,0.30,gpe-list@linuxtogo.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL


DEP_MODULES="gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 libgpewidget >= 0.114 dbus-glib-1 gnome-vfs-2.0 gnome-vfs-module-2.0"

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="es sl sv vi ko ja wa"

gpefilemanager_localedir='${datadir}/locale'
AC_SUBST(gpefilemanager_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


ENABLE_HILDON=false

AC_ARG_ENABLE(hildon,
[  --enable-hildon         Enable Hildon GUI],
[
	ENABLE_HILDON=true
        PKG_CHECK_MODULES(HILDON, hildon-1,
		[
			AC_DEFINE(HILDON_VER, 2, [Version of hildon libraries]),
			DEP_MODULES="$DEP_MODULES libhildonmime"
		],
                [
                        AC_DEFINE(HILDON_VER, 0, [Version of hildon libraries])
                        PKG_CHECK_MODULES(HILDON, hildon-libs >= 0.12.0)
			DEP_MODULES="$DEP_MODULES libossomime"
                ])
	DEP_MODULES="$DEP_MODULES libosso"
],
[
	ENABLE_HILDON=false
])

AC_SUBST(ENABLE_HILDON)

AM_CONDITIONAL(HILDON, test x$ENABLE_HILDON = xtrue)

PKG_CHECK_MODULES(DEPS, $DEP_MODULES)


AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_OUTPUT([
Makefile
po/Makefile.in
gpe-filemanager.desktop.in
hildon/gpe-filemanager.service
hildon/gpe-filemanager.desktop.in
])
