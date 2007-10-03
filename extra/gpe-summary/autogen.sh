#!/bin/sh

version=1.7
set -x
autoheader 

glib-gettextize --copy --force
libtoolize --automake --copy --force
intltoolize --automake --copy --force

aclocal-$version
autoconf
libtoolize
automake-$version --add-missing --foreign
