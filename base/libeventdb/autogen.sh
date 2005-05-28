libtoolize --copy --force

aclocal $ACLOCAL_FLAGS

automake -a $am_opt
autoconf
