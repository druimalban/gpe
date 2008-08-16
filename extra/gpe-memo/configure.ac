AC_INIT(gpe-memo, 0.2,[skyhusker@rm-fr.net])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AC_PROG_INSTALL
AC_PROG_CC
AC_LANG(C)

# i8n stuff
AH_TEMPLATE([GETTEXT_PACKAGE], [Package name for gettext])
GETTEXT_PACKAGE=$PACKAGE
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE")
AC_SUBST(GETTEXT_PACKAGE)
ALL_LINGUAS="$(ls ${srcdir}/po/*.po | sed 's/.*\/po\///;s/\.po//' | tr \\n ' ')"
gpememo_localedir='${prefix}/${DATADIRNAME}/locale'
AC_SUBST(gpememo_localedir)

AM_GLIB_GNU_GETTEXT
AC_PROG_INTLTOOL([0.23])


dnl Check for libgsm
AC_CHECK_HEADER(gsm/gsm.h, [AC_DEFINE(HAVE_GSM_GSM_H)],
[AC_CHECK_HEADER(gsm.h,[AC_DEFINE(HAVE_GSM_H)],AC_MSG_ERROR(Can't find gsm.h))])

dnl Check for GTK+
PKG_CHECK_MODULES(GTK, gtk+-2.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

dnl Check for GPE
PKG_CHECK_MODULES(GPE, libgpewidget >= 0.102)
AC_SUBST(GPE_CFLAGS)
AC_SUBST(GPE_LIBS)

dnl Check for eSound
PKG_CHECK_MODULES(ESD, esound)
AC_SUBST(ESD_CFLAGS)
AC_SUBST(ESD_LIBS)

dnl Generate the output files.
AC_CONFIG_FILES([
Makefile
src/Makefile
data/Makefile
po/Makefile.in
data/gpe-memo.desktop.in
])
AC_OUTPUT
