#!/bin/sh

intltoolize --copy --automake --force
autoreconf -f -i -s
