# gettext
PKG_NAME="gpe-mininet"
GETTEXTIZE="glib-gettextize"

touch po/ChangeLog

$GETTEXTIZE --version < /dev/null > /dev/null 2>&1
if test $? -ne 0; then
  echo
  echo "**Error**: You must have \`$GETTEXTIZE' installed" \
       "to compile $PKG_NAME."
  DIE=1
fi

intltoolize --version < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`intltoolize' installed" \
     "to compile $PKG_NAME."
  DIE=1
}

if test "$GETTEXTIZE"; then
 echo "Creating $dr/aclocal.m4 ..."
 test -r aclocal.m4 || touch aclocal.m4
 echo "Running $GETTEXTIZE...  Ignore non-fatal messages."
 echo "no" | $GETTEXTIZE --copy
 echo "Making aclocal.m4 writable ..."
 test -r aclocal.m4 && chmod u+w aclocal.m4
fi
echo "Running intltoolize..."
intltoolize --copy --automake

libtoolize --copy --force

aclocal $ACLOCAL_FLAGS

automake -a $am_opt
autoconf
./configure --enable-maintainer-mode "$@"

echo "Now type 'make' to compile"
