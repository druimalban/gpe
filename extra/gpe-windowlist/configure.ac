#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(gpe-windowlist, 0.1, gpe@handhelds.org)
AC_CONFIG_SRCDIR([main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])

PKG_CHECK_MODULES(DEPS, gtk+-2.0 libgpewidget libgpelaunch)

AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "$prefix/share/locale/$PACKAGE_NAME", [Directory for locale files])

AC_CONFIG_FILES([Makefile gpe-windowlist.desktop.in po/Makefile.in])
AC_OUTPUT
