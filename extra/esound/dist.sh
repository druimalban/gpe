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
install .libs/libesd.so.0.2.28 dist.libesd0/usr/lib
cp familiar/control.libesd0 dist.libesd0/CONTROL/control
cp familiar/postinst.libesd0 dist.libesd0/CONTROL/postinst
chown -R root.root dist.libesd0
ipkg-build dist.libesd0

mkdir -p dist.esddsp/CONTROL
mkdir -p dist.esddsp/usr/lib
mkdir -p dist.esddsp/usr/bin
install .libs/libesddsp.so.0.2.28 dist.esddsp/usr/lib
install esddsp dist.esddsp/usr/bin
cp familiar/control.esddsp dist.esddsp/CONTROL/control
cp familiar/postinst.esddsp dist.esddsp/CONTROL/postinst
chown -R root.root esddsp
ipkg-build dist.esddsp


