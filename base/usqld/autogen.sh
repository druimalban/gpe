#!/bin/sh
# Run this to generate all the initial makefiles, etc.

DIE=0


(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile sqlited."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^AM_PROG_XML_I18N_TOOLS" $srcdir/configure.in >/dev/null) && {
  (xml-i18n-toolize --version) < /dev/null > /dev/null 2>&1 || {
    echo 
    echo "**Error**: You must have \`xml-i18n-toolize' installed to compile sqlited."
    echo "Get ftp://ftp.sqlited.org/pub/sqlited/stable/sources/xml-i18n-tools/xml-i18n-tools-0.6.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.in >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile sqlited."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.2d.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

#grep "^AM_GNU_GETTEXT" $srcdir/configure.in >/dev/null && {
#  grep "sed.*POTFILES" $srcdir/configure.in >/dev/null || \
#  (gettext --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`gettext' installed to compile sqlited."
#    echo "Get ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

#grep "^AM_sqlited_GETTEXT" $srcdir/configure.in >/dev/null && {
#  grep "sed.*POTFILES" $srcdir/configure.in >/dev/null || \
#  (gettext --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`gettext' installed to compile sqlited."
#    echo "Get ftp://alpha.gnu.org/gnu/gettext-0.10.35.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile sqlited."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi


conf_flags="--enable-maintainer-mode --enable-compile-warnings" #--enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
