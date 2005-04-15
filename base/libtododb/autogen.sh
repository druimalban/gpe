libtoolize --copy --force

aclocal $ACLOCAL_FLAGS

automake -a $am_opt
autoconf
./configure --enable-maintainer-mode "$@"

echo "Fixing libtool..."
patch -p0 < libtool-cross.patch

echo "Now type 'make' to compile"
