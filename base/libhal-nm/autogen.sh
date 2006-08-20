#!/bin/sh
aclocal && autoheader && autoconf 
libtoolize -c -f
automake -ac
