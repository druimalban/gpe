#!/bin/sh

rm -rf dist.esd dist.libesd0

mkdir -p dist.esd/CONTROL
mkdir -p dist.esd/usr/sbin
install .libs/esd dist.esd/usr/sbin/
cp familiar/control.esd dist.esd/CONTROL/control
chown -R root.root dist.esd
ipkg-build dist.esd

mkdir -p dist.libesd0/CONTROL
mkdir -p dist.libesd0/usr/lib
install .libs/libesd.so.0.2.28 dist.esd/usr/lib
cp familiar/control.libesd0 dist.libesd0/CONTROL/control
cp familiar/postinst.libesd0 dist.libesd0/CONTROL
chown -R root.root dist.libesd0
ipkg-build dist.libesd0

