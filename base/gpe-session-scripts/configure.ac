dnl Process this file with autoconf to produce a configure script.
dnl gpe-session-scripts-0.68

AC_PREREQ(2.59)
AC_INIT(gpe-session-scripts,0.68,gpe-list@linuxtogo.org)
AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_PROG_INSTALL


# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""

gpesession-scripts_localedir='${prefix}/${datadir}/locale'
AC_SUBST(gpesession-scripts_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


AC_OUTPUT([
Makefile
po/Makefile.in
gpe-logout.desktop.in
])
